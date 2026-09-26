#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "esp_err.h"

typedef struct
{
    uint32_t bytes;
    uint32_t parity_err;
    uint32_t frame_err;
    uint32_t short_runs;
    uint32_t glitches;
    uint32_t edge_overflow;
    int32_t skew_us;    // retard compensé en cours
    uint32_t cal_score; // dernière calibration, en ‰ de paliers bien placés
    uint32_t cal_ok;
    uint32_t cal_ko;
} soft_rx_stats_t;

esp_err_t soft_rx_start(int gpio);
void soft_rx_stop(void);
void soft_rx_resume(void); // début de fenêtre de lecture
void soft_rx_pause(void);  // fin de fenêtre : plus d'interruptions, économie d'énergie
bool soft_rx_active(void);
size_t soft_rx_read(uint8_t *buf, size_t max);
void soft_rx_force_skew(int32_t us); // -1 : calibration automatique
int32_t soft_rx_get_skew(void);
void soft_rx_get_stats(soft_rx_stats_t *out, bool reset);
