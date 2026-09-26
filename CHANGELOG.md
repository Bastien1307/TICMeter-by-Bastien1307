# Changelog — TICMeter by Bastien1307

Modifications apportées au firmware de [GammaTroniques](https://github.com/GammaTroniques/TICMeter),
à partir de sa branche `main` (commit `bd2da3f`, novembre 2024, version V3.2.13 + 4 commits).

*Changes made to the GammaTroniques firmware, based on its `main` branch (commit `bd2da3f`).*

## V3.3.0-std — 2026-09-26

### Corrigé / Fixed

- **Lecture du mode TIC standard (9600 bauds)** : nouveau récepteur logiciel
  (`firmware/main/soft_rx.c`) qui remplace l'UART en mode standard. Il horodate les fronts du
  signal, calibre automatiquement le retard du démodulateur au début de chaque lecture, le
  compense et reconstruit les octets 7E1. Actif seulement pendant les fenêtres de lecture.
  Checksum errors: ~1,300 → 0–3 per reading.
  *Standard TIC mode reading: new software receiver replacing the UART, with automatic
  delay calibration.*
- **Verrou Zigbee** : `zigbee_send()` prend le verrou de la pile Zigbee autour de
  `esp_zb_zcl_set_attribute_val()`, du report d'attributs et de la relance d'appairage
  (`firmware/main/zigbee.c`). *Zigbee stack lock now taken for attribute updates.*

### Ajouté / Added

- **Libellés du mode standard nettoyés** (espaces de bord retirés, espaces doubles réduits),
  désactivable. Nouveau réglage NVS `std-raw-labels` (0 = nettoyé, défaut ; 1 = brut).
  *Standard-mode labels trimmed, configurable.*
- **Retard imposable** : réglage NVS `rx-skew` (0 = calibration automatique, défaut ;
  1..60 = retard fixe en µs). *Optional forced delay.*
- **Console en mode Zigbee**, lancée après la pile Zigbee et seulement si l'USB est branché.
  Commandes `soft-rx-stats`, `set-rx-skew`, `set-std-labels`.
  *Console available in Zigbee mode when USB is plugged.*
- Statistiques du récepteur logiciel dans le résumé de lecture (`Soft RX: ...`).

### Modifié / Changed

- Version **V3.3.0-std** (`PROJECT_VER`), fichier OTA Zigbee à la même version
  (`0x030300`) ; champ Zigbee *Date code* `AAAAMMJJ-std`.
- `PRODUCTION` toujours défini (auparavant seulement sur un commit tagué : une compilation
  hors tag embarquait le mode développement).
- Dépendance de compilation `htmlmin2` au lieu de `htmlmin` (Python 3.13).
