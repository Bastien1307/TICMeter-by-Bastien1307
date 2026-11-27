# Changelog — TICMeter by Bastien1307

Modifications apportées au firmware de [GammaTroniques](https://github.com/GammaTroniques/TICMeter),
à partir de sa branche `main` (commit `bd2da3f`, novembre 2024, version V3.2.13 + 4 commits).

*Changes made to the GammaTroniques firmware, based on its `main` branch (commit `bd2da3f`).*

## V3.4.15-std — 2026-11-28

Version de diagnostic Wi-Fi, proposée par [DjMomo](https://github.com/DjMomo) (PR #1). Aucun
changement pour le mode Zigbee.
*Wi-Fi diagnostics release, contributed by DjMomo (PR #1). No change in Zigbee mode.*

### Ajouté / Added

- **Journal Wi-Fi plus parlant.** Quand la connexion échoue, la console indique maintenant la
  borne réellement utilisée (BSSID), la force du signal, le canal, et si le module a obtenu une
  adresse IP (état du client DHCP). La perte de l'adresse IP est aussi signalée. Le
  fonctionnement du Wi-Fi ne change pas, seules des lignes de journal sont ajoutées.
  *More helpful Wi-Fi log: access point (BSSID), signal strength, channel and DHCP state on a
  failed connection, and IP loss reported. Wi-Fi behaviour is unchanged.*

## V3.4.14-std — 2026-11-28

Validée sur notre TICMeter (Linky monophasé, mode standard, HP/HC, Zigbee for Domoticz) pendant
5 jours, sans redémarrage ni perte de liaison, avec toutes les bascules HP/HC à l'heure.
*Validated on our TICMeter for 5 days, without restart or lost link, every tariff change on time.*

### Ajouté / Added

- **Réglage du récepteur logiciel** (mode standard), sur la page de configuration et en console
  (`set-soft-rx <0|1|2>`). **Auto** (par défaut) démarre en lecture classique et passe au
  récepteur logiciel si les lectures sont abîmées ou incomplètes. **Toujours** reprend le
  comportement des versions précédentes. **Jamais** garde la lecture classique seule, comme le
  firmware officiel.
  *Software receiver setting (standard mode), on the configuration page and in the console.
  Auto (default) switches to the software receiver when readings are corrupted or incomplete,
  Always keeps the previous behaviour, Never uses the plain UART like the official firmware.*
- **Diagnostic des redémarrages**, désactivé par défaut (page de configuration ou
  `set-diag <0|1>`). Une fois activé, le TICMeter envoie en Zigbee la cause de son dernier
  démarrage (`ff42/00f0`), l'adresse et la tâche d'un éventuel plantage (`00f1`, `00f2`) et
  l'état de la lecture du compteur (`00f3`). Pour garder la trace d'un plantage, la table des
  partitions gagne une partition `coredump` en fin de mémoire. La page de mise à jour l'écrit
  sans déplacer les autres partitions, donc réglages et appairage Zigbee sont conservés. Sans
  elle (application seule flashée avec esptool), le module fonctionne normalement mais ne garde
  pas de trace des plantages.
  *Restart diagnostics, off by default (reset reason, crash address and task, reading status sent
  over Zigbee). The partition table gains a `coredump` partition at the end of the flash. The
  update page writes it without moving the other partitions, so settings and Zigbee pairing are
  kept.*
- **Profil du lendemain (`PJOURF+1`) envoyé en entier** (6 plages au plus) au lieu d'être coupé
  à 16 caractères.
  *Next-day profile (`PJOURF+1`) sent in full instead of being cut at 16 characters.*
- **Durée de fonctionnement calée sur l'horloge du Linky** (mode standard). Elle dérivait de
  plus de 25 % pendant les mises en veille.
  *Uptime based on the Linky clock (standard mode). It drifted by more than 25 % during sleep.*
- **Console USB disponible en branchant le câble sur un module en marche**, sans le redémarrer
  (il doit tourner depuis plus de 60 s). Le débranchement le redémarre. Nouvelle commande
  `filet-uart`.
  *USB console available when the cable is plugged into a running module (up for more than 60 s),
  without restarting it. Unplugging restarts it. New `filet-uart` command.*

### Corrigé / Fixed

- **Module figé pendant des heures.** Une rafale d'erreurs sur la ligne TIC en mode standard
  bloquait la tâche de lecture jusqu'au déclenchement du chien de garde. Les interruptions
  d'erreur de la lecture classique sont désormais coupées en mode standard.
  *Module frozen for hours. A burst of TIC line errors in standard mode starved the system until
  the watchdog fired. UART error interrupts are now disabled in standard mode.*
- **Plantage au changement de mode de lecture** (standard ou historique, en mode Auto). La file
  de la lecture classique était libérée deux fois.
  *Crash when switching reading mode (standard or historical). The UART queue was freed twice.*
- **Valeurs jamais envoyées jusqu'au redémarrage.** Un attribut Zigbee n'était créé que si sa
  valeur était déjà lue au démarrage, et une première lecture incomplète le laissait absent.
  *Values never sent until the next restart. A Zigbee attribute was only created if its value had
  already been read at startup.*
- **Valeurs perdues dans les rafales d'envoi.** Une pause de 80 ms sépare désormais deux envois
  Zigbee.
  *Values lost in report bursts. An 80 ms pause now separates two Zigbee reports.*
- **Lecture du compteur arrêtée** (index figés, puis module injoignable). La file de la lecture
  classique pouvait être perdue. Elle est désormais restaurée sans redémarrage.
  *Meter reading stopped (frozen values, then unreachable). The UART queue could be lost, it is
  now restored without restarting.*
- **Valeurs texte abîmées par un bit en trop** ("PAS DE ` MESSAGE", numéro de compteur avec une
  lettre minuscule), que la somme de contrôle TIC ne voit pas. Une valeur contenant un caractère
  de code 0x60 ou plus, qu'aucune valeur TIC n'utilise, est rejetée et l'ancienne conservée.
  *Text values corrupted by an extra bit, unseen by the TIC checksum. Values containing a
  character of code 0x60 or above are discarded and the previous value kept.*
- **Robustesse du décodage.** Les copies dans les champs de la trame sont bornées (un séparateur
  abîmé pouvait déborder la pile), et une lecture hors tableau est supprimée quand une trame ne
  contient aucun groupe.
  *Decoding robustness. Bounded copies into frame fields, and an out-of-bounds read removed.*

## V3.4.10-std — 2026-10-11

### Ajouté / Added

- **Valeurs fixes du compteur envoyées en Zigbee** (puissance souscrite `PREF`, puissance de
  coupure `PCOUP`, numéro de compteur `ADSC`, point de livraison `PRM`, nom du calendrier
  `NGTF`…) : elles partent une fois au démarrage, puis à chaque changement. Auparavant elles
  n'étaient connues du coordinateur qu'à l'appairage : après un changement de contrat (9 kVA
  vers 6 kVA par exemple), l'ancienne valeur restait affichée.
  *Static meter values (subscribed power, meter serial number, tariff calendar name…) are now
  reported over Zigbee at startup and whenever they change, instead of only at pairing.*
- **Relève rapide après chaque envoi** : pendant 10 s après avoir envoyé ses mesures, le TICMeter
  interroge son routeur en continu. Les demandes de lecture du coordinateur arrivées dans cette
  fenêtre reçoivent toutes une réponse en moins d'une seconde (environ une sur deux avant, le
  routeur ne gardant les messages que quelques secondes pour un appareil en veille).
  *Fast polling for 10 s after each report: read requests sent by the coordinator in that window
  are all answered within a second (about one in two before). Tested with Zigbee for Domoticz,
  not with Zigbee2MQTT.*

### Corrigé / Fixed

- **Index qui baisse ou tronqué** : un chiffre pouvait arriver avec le bit 0x40 en trop et
  devenir une lettre (« 003242060 » reçu « 0032420p0 »), ce que la somme de contrôle TIC ne voit
  pas. La valeur était alors lue jusqu'à la lettre, donc tronquée. Une valeur numérique qui
  contient autre chose que des chiffres est désormais rejetée et l'ancienne valeur conservée
  (modes standard et historique).
  *Index going down or truncated: a digit could be received as a letter (one extra bit on the TIC
  line, invisible to the checksum) and the value was cut at that letter. Numeric values containing
  anything but digits are now discarded (standard and historic modes).*
- **Nom du calendrier tarifaire (`NGTF`) jamais reçu** par le coordinateur : il est maintenant
  envoyé par le TICMeter au lieu d'attendre une lecture qui restait sans réponse.
  *Tariff calendar name (`NGTF`) never received by the coordinator: it is now reported by the
  device instead of waiting for a read request that was never answered.*

## V3.4.7-std — 2026-10-04

### Ajouté / Added

- **Bascule HP/HC détectée tout de suite** : le firmware décode le profil du lendemain envoyé
  par le Linky (`PJOURF+1`, auparavant tronqué à 16 caractères) et se réveille 30 s avant chaque
  changement de tarif. Il lit en continu jusqu'au changement, puis envoie aussitôt le nouveau
  tarif (`LTARF`) et l'heure du compteur, sans attendre l'envoi complet. Dans Domoticz, la
  bascule apparaît en quelques secondes au lieu d'environ 1 min 15.
  Commande console `bascule-info` : prochaines bascules et prochain réveil.
  *Tariff changes (peak / off-peak) are now reported within seconds: the firmware decodes the
  next-day profile, wakes up 30 s before each change and sends the new tariff immediately.*

### Corrigé / Fixed

- **Libellé du tarif parfois abîmé** (« HEURE CREUSE » reçu « HEU?E CREUSE », vu « Unknown
  Tarif » par Zigbee for Domoticz, environ une fois par jour) : une lettre arrivait avec son
  bit 0x40 à zéro. La somme de contrôle TIC ne porte que sur 6 bits et ne voit pas cette erreur.
  Les lignes contenant un caractère de contrôle sont désormais rejetées.
  *Tariff label occasionally corrupted (one bit lost on the TIC line, invisible to the 6-bit
  checksum): lines containing control characters are now discarded.*
- Envoi Zigbee des textes : la chaîne au format Zigbee est construite dans une copie, le tampon
  lu par le décodage TIC n'est plus modifié pendant l'envoi.
  *Zigbee string attributes are built in a local copy instead of the shared buffer.*

## Page web — mises à jour du 2026-09-26

- Barre du haut permanente : état, Connecter / Déconnecter, Redémarrer (seulement si un réglage l'exige), progression (aussi recopiée en section 2).
- Barre du bas : soutien (facultatif) et bascule **français / anglais** (`?lang=en` pour ouvrir en anglais).
- Journal : une seule ligne par commande. Fichier OTA Zigbee retiré des releases (jamais testé).
  *Top and bottom bars, French / English switch, cleaner log.*

## V3.3.1-std — 2026-09-26

### Corrigé / Fixed

- `soft-rx-stats` : le score et le nombre de calibrations ne sont plus remis à zéro à chaque
  résumé de lecture (seuls les compteurs du cycle le sont).
  *Calibration score no longer reset by the reading summary.*

## Page web — 2026-09-26

- Page unique (`docs/`, publiée sur GitHub Pages) pour connecter le TICMeter en USB depuis le
  navigateur : état, sauvegarde et restauration de la flash, mise à jour (sans effacer la
  configuration), réglages. S'appuie sur esptool-js (Espressif, Apache 2.0).
  *Single web page to connect, back up, update and configure the TICMeter over USB.*

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
