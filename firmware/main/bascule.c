/**
 * bascule.c — réveil programmé à l'heure des changements de tarif (mode TIC standard)
 *
 * Ajout de Bastien1307 (2026) au firmware TICMeter de GammaTroniques.
 * Licence CC BY-NC 4.0, comme le projet d'origine.
 *
 * Le Linky annonce dans PJOURF+1 le programme du jour fournisseur suivant : jusqu'à 11 blocs
 * « HHMMSSSS » (heure de début, code d'action sur 16 bits), les blocs vides valant « NONUTILE ».
 * Exemple (HP/HC) : 00008002 0054C001 06248002 1424C001 16548002
 * Le premier bloc à 00h00 donne l'état au début de la journée ; les suivants sont des changements.
 *
 * Le profil du jour en cours n'est pas transmis : on garde celui annoncé la veille. Au premier
 * démarrage, on utilise à défaut le profil du lendemain (identique chaque jour pour un contrat
 * heures pleines / heures creuses classique).
 *
 * L'heure vient du Linky (champ DATE), convertie sans fuseau : (date % 86400) = secondes
 * depuis minuit, heure locale du compteur.
 */
#include <string.h>
#include <stdio.h>
#include <inttypes.h>
#include "esp_timer.h"
#include "esp_log.h"
#include "bascule.h"

static const char *TAG = "BASCULE";

#define PROFIL_MAX 128
#define SECONDES_JOUR 86400

static char profil_recu[PROFIL_MAX];        // PJOURF+1 de la lecture en cours, appliqué par bascule_maj
static char profil_demain[PROFIL_MAX];      // dernier PJOURF+1 retenu
static int32_t jour_profil_demain = -1;     // jour (depuis 1970) où il a été reçu
static char profil_aujourdhui[PROFIL_MAX];  // PJOURF+1 reçu la veille
static int32_t jour_profil_aujourdhui = -1; // jour auquel il s'applique

static time_t date_linky = 0;         // dernière heure lue sur le Linky
static int32_t heure_visee = -1;      // prochaine bascule (secondes depuis minuit), vue par bascule_secondes_avant
static int32_t jour_vise = -1;        // son jour (depuis 1970)
static int32_t heure_traitee = -1;    // bascule déjà vue (le Linky a pu basculer avant l'heure) : ne pas la guetter deux fois
static int32_t jour_traite = -1;
static int32_t dernier_ecart = INT32_MIN; // retard (+) ou avance (-) du Linky sur l'heure annoncée, en secondes
static int64_t horloge_lecture_us = 0; // esp_timer au moment de cette lecture

void bascule_set_profil(const char *valeur)
{
    // mis en attente : au changement de jour, l'ancien profil doit d'abord devenir celui du jour
    strncpy(profil_recu, valeur, PROFIL_MAX - 1);
    profil_recu[PROFIL_MAX - 1] = '\0';
}

void bascule_maj(time_t date)
{
    if (date <= 0)
        return;
    date_linky = date;
    horloge_lecture_us = esp_timer_get_time();
    int32_t jour = (int32_t)(date / SECONDES_JOUR);

    if (jour_profil_demain >= 0 && jour_profil_demain < jour)
    {
        // le profil annoncé un jour précédent pour « le lendemain » s'applique aujourd'hui
        memcpy(profil_aujourdhui, profil_demain, PROFIL_MAX);
        jour_profil_aujourdhui = jour;
        ESP_LOGI(TAG, "New day: today's tariff profile = %s", profil_aujourdhui);
    }
    if (profil_recu[0])
    {
        memcpy(profil_demain, profil_recu, PROFIL_MAX);
        jour_profil_demain = jour;
        profil_recu[0] = '\0';
    }
}

// Heures de changement (secondes depuis minuit, triées) d'un profil ; le bloc de 00h00 est exclu.
static int lire_changements(const char *profil, int32_t *secondes, int max)
{
    int n = 0;
    const char *p = profil;
    while (*p && n < max)
    {
        while (*p == ' ')
            p++;
        if (strlen(p) < 8)
            break;
        if (strncmp(p, "NONUTILE", 8) != 0)
        {
            int hh = (p[0] - '0') * 10 + (p[1] - '0');
            int mm = (p[2] - '0') * 10 + (p[3] - '0');
            if (hh >= 0 && hh < 24 && mm >= 0 && mm < 60 && (hh || mm))
            {
                secondes[n++] = hh * 3600 + mm * 60;
            }
        }
        p += 8;
    }
    return n; // les blocs du Linky sont déjà dans l'ordre chronologique
}

int32_t bascule_secondes_avant(void)
{
    if (!date_linky)
        return -1;
    const char *aujourdhui = profil_aujourdhui[0] ? profil_aujourdhui : profil_demain;
    if (!aujourdhui[0])
        return -1;

    int32_t maintenant = (int32_t)(date_linky % SECONDES_JOUR) +
                         (int32_t)((esp_timer_get_time() - horloge_lecture_us) / 1000000);
    int32_t changements[11];

    int32_t jour = (int32_t)(date_linky / SECONDES_JOUR);

    int n = lire_changements(aujourdhui, changements, 11);
    for (int i = 0; i < n; i++)
    {
        if (changements[i] > maintenant && !(changements[i] == heure_traitee && jour == jour_traite))
        {
            heure_visee = changements[i];
            jour_vise = jour;
            return changements[i] - maintenant;
        }
    }
    // plus rien aujourd'hui : premier changement de demain
    if (profil_demain[0])
    {
        n = lire_changements(profil_demain, changements, 11);
        for (int i = 0; i < n; i++)
        {
            if (changements[i] == heure_traitee && jour + 1 == jour_traite)
                continue;
            heure_visee = changements[i];
            jour_vise = jour + 1;
            return SECONDES_JOUR - maintenant + changements[i];
        }
    }
    return -1;
}

void bascule_noter(time_t date)
{
    if (heure_visee < 0 || date <= 0)
        return;
    int32_t ecart = (int32_t)(date % SECONDES_JOUR) - heure_visee;
    if (ecart > SECONDES_JOUR / 2)
        ecart -= SECONDES_JOUR;
    else if (ecart < -SECONDES_JOUR / 2)
        ecart += SECONDES_JOUR;
    dernier_ecart = ecart;
    heure_traitee = heure_visee;
    jour_traite = jour_vise;
    ESP_LOGI(TAG, "Tariff change seen: expected %02" PRId32 ":%02" PRId32 ", Linky offset %+" PRId32 " s",
             heure_visee / 3600, (heure_visee / 60) % 60, ecart);
}

void bascule_afficher(void)
{
    int32_t s = bascule_secondes_avant();
    printf("Profil du jour : %s\n", profil_aujourdhui[0] ? profil_aujourdhui : "(inconnu, profil du lendemain utilisé)");
    printf("Profil du lendemain : %s\n", profil_demain[0] ? profil_demain : "(pas encore reçu)");
    if (date_linky)
    {
        int32_t m = (int32_t)(date_linky % SECONDES_JOUR);
        printf("Heure Linky à la dernière lecture : %02" PRId32 ":%02" PRId32 ":%02" PRId32 "\n", m / 3600, (m / 60) % 60, m % 60);
    }
    if (dernier_ecart != INT32_MIN)
        printf("Dernière bascule vue : %02" PRId32 ":%02" PRId32 ", le Linky avait %" PRId32 " s %s\n",
               heure_traitee / 3600, (heure_traitee / 60) % 60, dernier_ecart < 0 ? -dernier_ecart : dernier_ecart,
               dernier_ecart < 0 ? "d'avance" : "de retard");
    if (s < 0)
        printf("Prochaine bascule : inconnue\n");
    else
        printf("Prochaine bascule dans %" PRId32 " s (%02" PRId32 " min %02" PRId32 " s)\n", s, s / 60, s % 60);
}
