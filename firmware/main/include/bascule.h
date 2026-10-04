#pragma once
#include <stdint.h>
#include <time.h>

// Guet de la bascule : réveil BASCULE_AVANCE_S avant l'heure annoncée, lecture continue du Linky
// jusqu'au changement de tarif (NTARF), au plus BASCULE_ATTENTE_APRES_S après l'heure annoncée.
// Le Linky ne bascule pas pile à l'heure (constaté le 2026-09-27 : passage HP -> HC entre +3 s et +22 s).
#define BASCULE_AVANCE_S 30
#define BASCULE_ATTENTE_APRES_S 60

void bascule_set_profil(const char *valeur); // PJOURF+1 complet, à chaque décodage
void bascule_maj(time_t date);               // heure du Linky (DATE) après chaque lecture
int32_t bascule_secondes_avant(void);        // secondes avant la prochaine bascule, -1 si inconnu
void bascule_noter(time_t date);             // changement de tarif vu dans la trame datée « date »
void bascule_afficher(void);                 // résumé pour la console
