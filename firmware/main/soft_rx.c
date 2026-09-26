/**
 * soft_rx.c — récepteur TIC logiciel (mode standard, 9600 bauds 7E1)
 *
 * Ajout de Bastien1307 (2026) au firmware TICMeter de GammaTroniques.
 * Licence CC BY-NC 4.0, comme le projet d'origine.
 *
 * Le démodulateur du TICMeter remonte en retard : chaque niveau haut arrive
 * raccourci d'environ 40 µs et le niveau bas voisin rallongé d'autant. L'UART,
 * qui échantillonne au milieu du bit, lit alors des 1 à la place de 0.
 * Ici on horodate chaque front, on compense ce retard, puis on arrondit chaque
 * palier à un nombre entier de bits avant de reconstruire les octets.
 */
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/stream_buffer.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_attr.h"
#include "esp_log.h"
#include "soft_rx.h"
#include "config.h"

static const char *TAG = "SOFT_RX";

#define EDGE_RING 4096        // fronts mémorisés (~0,4 s à 9600 bauds)
#define BIT_US_X100 10417     // durée d'un bit à 9600 bauds, en centièmes de µs
#define SKEW_DEFAULT_US 40    // retard par défaut du front montant (mesuré sur le premier module)
#define SKEW_MAX_US 60        // plage explorée par la calibration
#define CAL_RUNS 600          // paliers mis de côté pour calibrer à chaque fenêtre de lecture
#define CAL_MAX_RUN_US 700    // au-delà : ligne au repos, inutile pour calibrer
#define GLITCH_US 12          // paliers plus courts : parasites, fusionnés
#define IDLE_FLUSH_US 3000    // ligne haute plus longtemps : fin de caractère
#define MAX_RUN_BITS 12

static uint32_t edge_ts[EDGE_RING];
static uint8_t edge_lvl[EDGE_RING];
static volatile uint32_t edge_head = 0;
static uint32_t edge_tail = 0;
static volatile uint32_t edge_overflow = 0;

static int rx_gpio = -1;
static StreamBufferHandle_t rx_stream = NULL;
static TaskHandle_t rx_task = NULL;
static volatile bool rx_enabled = false; // mode STD : le récepteur logiciel remplace l'UART
static volatile bool rx_running = false; // interruptions actives (fenêtre de lecture seulement)

static soft_rx_stats_t stats;

// calibration : retard compensé, recalculé au début de chaque fenêtre de lecture
static int32_t skew_us = SKEW_DEFAULT_US;
static int32_t skew_forced_us = -1; // >= 0 : valeur imposée (console), pas de calibration
static uint16_t cal_dur[CAL_RUNS];
static uint8_t cal_lvl[CAL_RUNS];
static uint32_t cal_count = 0;
static bool cal_done = false;

// état du décodeur de trame série
static int8_t bit_index = -1; // -1 : attente du bit de start
static uint8_t cur_byte = 0;
static uint8_t cur_ones = 0;

static void IRAM_ATTR soft_rx_isr(void *arg)
{
    uint32_t head = edge_head;
    uint32_t next = (head + 1) % EDGE_RING;
    if (next == edge_tail)
    {
        edge_overflow++;
        return;
    }
    edge_ts[head] = (uint32_t)esp_timer_get_time();
    edge_lvl[head] = gpio_get_level(rx_gpio);
    edge_head = next;
}

static void emit(uint8_t c)
{
    xStreamBufferSend(rx_stream, &c, 1, 0);
    stats.bytes++;
}

static void feed_bit(uint8_t b)
{
    if (bit_index < 0)
    {
        if (b == 0) // bit de start
        {
            bit_index = 0;
            cur_byte = 0;
            cur_ones = 0;
        }
        return;
    }
    if (bit_index < 7) // 7 bits de données, poids faible en premier
    {
        cur_byte |= (b << bit_index);
        cur_ones += b;
        bit_index++;
        return;
    }
    if (bit_index == 7) // parité paire
    {
        if (((cur_ones + b) & 1) != 0)
            stats.parity_err++;
        bit_index++;
        return;
    }
    // bit de stop
    if (b == 1)
        emit(cur_byte);
    else
        stats.frame_err++;
    bit_index = -1;
}

static void decode_run(uint8_t level, int32_t dur_us)
{
    dur_us += level ? skew_us : -skew_us;
    int32_t n = (dur_us * 100 + BIT_US_X100 / 2) / BIT_US_X100;
    if (n < 1)
    {
        n = 1;
        stats.short_runs++;
    }
    if (n > MAX_RUN_BITS)
        n = MAX_RUN_BITS;
    for (int32_t i = 0; i < n; i++)
        feed_bit(level);
}

// part des paliers qui tombent à moins d'un quart de bit d'un nombre entier de bits, en ‰
static uint32_t cal_score(int32_t c)
{
    uint32_t ok = 0, total = 0;
    for (uint32_t i = 0; i < cal_count; i++)
    {
        if (cal_dur[i] > CAL_MAX_RUN_US)
            continue;
        int32_t d = (int32_t)cal_dur[i] * 100 + (cal_lvl[i] ? c : -c) * 100;
        int32_t n = (d + BIT_US_X100 / 2) / BIT_US_X100;
        int32_t err = d - n * BIT_US_X100;
        total++;
        if (n >= 1 && err <= BIT_US_X100 / 4 && err >= -BIT_US_X100 / 4)
            ok++;
    }
    return total ? ok * 1000 / total : 0;
}

static void calibrate(void)
{
    static uint32_t scores[SKEW_MAX_US + 1];
    uint32_t best = 0;
    for (int32_t c = 0; c <= SKEW_MAX_US; c++)
    {
        scores[c] = cal_score(c);
        if (scores[c] > best)
            best = scores[c];
    }
    // milieu du plateau des meilleures valeurs : le plus loin possible des erreurs
    int32_t lo = -1, hi = -1;
    for (int32_t c = 0; c <= SKEW_MAX_US; c++)
    {
        if (scores[c] + 5 >= best)
        {
            if (lo < 0)
                lo = c;
            hi = c;
        }
    }
    stats.cal_score = best;
    if (best >= 900 && lo >= 0)
    {
        skew_us = (lo + hi) / 2;
        stats.cal_ok++;
    }
    else
    {
        stats.cal_ko++; // signal trop mauvais : on garde la valeur précédente
    }
    stats.skew_us = skew_us;
    ESP_LOGI(TAG, "Calibration : retard %ld us (plateau %ld-%ld us, score %lu pour mille)", skew_us, lo, hi, best);
}

static void feed_run(uint8_t level, int32_t dur_us)
{
    if (!cal_done)
    {
        cal_dur[cal_count] = dur_us > 0xFFFF ? 0xFFFF : dur_us;
        cal_lvl[cal_count] = level;
        cal_count++;
        if (cal_count < CAL_RUNS)
            return;
        if (skew_forced_us < 0)
            calibrate();
        cal_done = true;
        for (uint32_t i = 0; i < cal_count; i++)
            decode_run(cal_lvl[i], cal_dur[i]);
        return;
    }
    decode_run(level, dur_us);
}

static void soft_rx_task(void *arg)
{
    bool have_run = false;
    uint8_t run_level = 1;
    uint32_t run_start = 0;
    bool idle_flushed = false;

    for (;;)
    {
        vTaskDelay(pdMS_TO_TICKS(5));
        if (!rx_enabled || !rx_running)
        {
            edge_tail = edge_head;
            have_run = false;
            bit_index = -1;
            continue;
        }
        while (edge_tail != edge_head)
        {
            uint32_t ts = edge_ts[edge_tail];
            uint8_t lvl = edge_lvl[edge_tail];
            edge_tail = (edge_tail + 1) % EDGE_RING;

            if (!have_run)
            {
                have_run = true;
                run_level = lvl;
                run_start = ts;
                idle_flushed = false;
                continue;
            }
            if (lvl == run_level) // front parasite : même niveau, on continue le palier
                continue;
            // un palier très court n'est pas un vrai bit : on l'absorbe dans le palier en cours
            if (edge_tail != edge_head && (edge_ts[edge_tail] - ts) < GLITCH_US && edge_lvl[edge_tail] == run_level)
            {
                edge_tail = (edge_tail + 1) % EDGE_RING;
                stats.glitches++;
                continue;
            }
            if (!idle_flushed)
                feed_run(run_level, ts - run_start);
            run_level = lvl;
            run_start = ts;
            idle_flushed = false;
        }
        // ligne au repos depuis longtemps : terminer le dernier caractère
        if (have_run && run_level == 1 && !idle_flushed &&
            ((uint32_t)esp_timer_get_time() - run_start) > IDLE_FLUSH_US)
        {
            for (int i = 0; i < MAX_RUN_BITS; i++)
                feed_bit(1);
            idle_flushed = true;
        }
        stats.edge_overflow = edge_overflow;
    }
}

esp_err_t soft_rx_start(int gpio)
{
    if (rx_task != NULL)
    {
        rx_enabled = true;
        return ESP_OK;
    }
    rx_gpio = gpio;
    rx_stream = xStreamBufferCreate(8 * 1024, 1);
    if (rx_stream == NULL)
        return ESP_ERR_NO_MEM;

    esp_err_t ret = gpio_install_isr_service(ESP_INTR_FLAG_IRAM);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE)
    {
        ESP_LOGE(TAG, "gpio_install_isr_service: %s", esp_err_to_name(ret));
        return ret;
    }
    gpio_set_intr_type(gpio, GPIO_INTR_ANYEDGE);
    ret = gpio_isr_handler_add(gpio, soft_rx_isr, NULL);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "gpio_isr_handler_add: %s", esp_err_to_name(ret));
        return ret;
    }
    gpio_intr_disable(gpio);
    if (config_values.rx_skew > 0 && config_values.rx_skew <= SKEW_MAX_US)
    {
        soft_rx_force_skew(config_values.rx_skew);
        ESP_LOGW(TAG, "Retard imposé par la config : %d us (calibration désactivée)", config_values.rx_skew);
    }
    xTaskCreate(soft_rx_task, "soft_rx", 4 * 1024, NULL, 13, &rx_task);
    rx_enabled = true;
    ESP_LOGI(TAG, "Récepteur logiciel actif sur GPIO%d", gpio);
    return ESP_OK;
}

void soft_rx_stop(void)
{
    soft_rx_pause();
    rx_enabled = false;
}

void soft_rx_resume(void)
{
    if (!rx_enabled || rx_running)
        return;
    xStreamBufferReset(rx_stream);
    cal_count = 0;
    cal_done = false;
    bit_index = -1;
    rx_running = true;
    gpio_intr_enable(rx_gpio);
}

void soft_rx_pause(void)
{
    if (!rx_running)
        return;
    gpio_intr_disable(rx_gpio);
    rx_running = false;
}

bool soft_rx_active(void)
{
    return rx_enabled;
}

size_t soft_rx_read(uint8_t *buf, size_t max)
{
    if (rx_stream == NULL || max == 0)
        return 0;
    return xStreamBufferReceive(rx_stream, buf, max, 0);
}

void soft_rx_force_skew(int32_t us)
{
    skew_forced_us = us;
    if (us >= 0)
        skew_us = us;
    stats.skew_us = skew_us;
}

int32_t soft_rx_get_skew(void)
{
    return skew_us;
}

void soft_rx_get_stats(soft_rx_stats_t *out, bool reset)
{
    *out = stats;
    if (reset)
    {
        uint32_t ovf = stats.edge_overflow;
        memset(&stats, 0, sizeof stats);
        stats.edge_overflow = ovf;
        stats.skew_us = skew_us;
    }
}
