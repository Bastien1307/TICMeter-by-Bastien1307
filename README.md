# TICMeter by Bastien1307 (non officiel / unofficial)

[![licence](https://img.shields.io/badge/License-CC_BY--NC_4.0-lightgrey.svg?style=for-the-badge)](https://creativecommons.org/licenses/by-nc/4.0/)
[![dernière version / latest release](https://img.shields.io/github/v/release/Bastien1307/TICMeter-by-Bastien1307?style=for-the-badge&label=version)](https://github.com/Bastien1307/TICMeter-by-Bastien1307/releases/latest)

> **Version non officielle** du firmware du [TICMeter de GammaTroniques](https://github.com/GammaTroniques/TICMeter).
> GammaTroniques n'est ni l'auteur ni responsable de ces modifications et n'apporte aucun support sur cette version.
>
> *Unofficial build of the [GammaTroniques TICMeter](https://github.com/GammaTroniques/TICMeter) firmware.
> GammaTroniques is neither the author of these changes nor responsible for them. English below.*

## 📦 Dernière version / Latest release

### 👉 **[Releases GitHub](https://github.com/Bastien1307/TICMeter-by-Bastien1307/releases)** — dernière / latest : **[V3.4.10-std](https://github.com/Bastien1307/TICMeter-by-Bastien1307/releases/latest)** (2026-10-11)

Chaque release donne ses changements et les fichiers à télécharger (`TICMeter.bin`,
`storage.bin`, `ota_data_initial.bin`, `SHA256SUMS.txt`). La page web ci-dessous installe
directement la dernière version.

*Each release lists its changes and the files to download. The web page below installs the
latest release directly.*

## 🌐 Page web : connexion, mise à jour et réglages / Web page: connect, update, configure

### 👉 **https://bastien1307.github.io/TICMeter-by-Bastien1307/**

Branchez le TICMeter en USB, ouvrez la page dans **Chrome, Edge ou Opera** (ordinateur), cliquez
sur « Connecter le TICMeter » :

- **État** : version du firmware, mode de communication, mode TIC, contrat, dernière lecture ;
- **Sauvegarde / restauration** de la flash complète ;
- **Mise à jour** vers la dernière version (configuration et appairage Zigbee conservés) ;
- **Réglages** : mode de communication (Zigbee, MQTT, Web, Tuya si le TICMeter a ses clés),
  mode TIC, intervalle d'envoi, Wi-Fi, MQTT, libellés et récepteur du mode standard ;
- **Console** : journal du TICMeter, copiable ou enregistrable.

*English version: **https://bastien1307.github.io/TICMeter-by-Bastien1307/?lang=en** —
plug the TICMeter over USB, open the page in Chrome, Edge or Opera (desktop) and click
"Connect": status, flash backup / restore, update (settings and Zigbee pairing kept),
configuration and console, all from the browser.*

---

## 🇫🇷 Français

### Le problème corrigé

Avec un Linky en **mode TIC standard** (9600 bauds), certains TICMeter lisent des trames
massivement corrompues : plus de mille erreurs de contrôle (checksum) par lecture, une
dizaine de champs seulement décodés sur une quarantaine, contrat « INCONNU », index qui
perdent un chiffre, et en Zigbee des attributs qui manquent ou des envois qui échouent.
En **mode historique** (1200 bauds), tout fonctionne.

### La cause

Mesures faites sur la broche d'entrée : l'étage qui démodule le signal du Linky
**remonte en retard**. Chaque niveau haut arrive raccourci d'environ 40 µs, et le niveau bas
voisin rallongé d'autant. À 1200 bauds (833 µs par bit), c'est sans effet. À 9600 bauds
(104 µs par bit), l'UART de l'ESP32, qui lit chaque bit en son milieu, tombe à côté :
des « 1 » sont lus comme des « 0 », toujours sur les mêmes caractères.

Le décalage est régulier : **rien n'est perdu**, les durées restent parfaitement séparables.

### La correction

En mode standard, un **récepteur logiciel** remplace l'UART :

- il horodate chaque front du signal sur la broche d'entrée ;
- il **mesure lui-même le retard** au début de chaque lecture (calibration automatique,
  de 0 à 60 µs) et le compense ;
- il reconstruit les octets (7 bits, parité paire) et les passe au décodeur d'origine.

Il n'est actif que pendant les fenêtres de lecture, pour ne pas peser sur l'énergie
fournie par le Linky. Le mode historique utilise toujours l'UART, sans changement.

**Résultat mesuré** : de ~1 300 erreurs de checksum par lecture à **0 à 3**, 41 champs
décodés au lieu de 11, contrat reconnu, bascule heures pleines / heures creuses remontée
dans Domoticz (plugin Zigbee for Domoticz), sur l'alimentation du Linky seule.

### Autres changements

- **Libellés du mode standard nettoyés** : le Linky envoie des textes centrés sur
  16 caractères (`  HEURE  PLEINE  `). Les espaces en bord sont retirés et les espaces
  doubles réduits (`HEURE PLEINE`) : les mots ne changent pas, seul le remplissage part.
  Sans ça, Zigbee for Domoticz ne reconnaît ni le tarif en cours ni le contrat.
  **Réglable** : `set-std-labels 1` rend le texte brut du Linky.
- **Console disponible en mode Zigbee**, seulement quand l'USB est branché (aucun coût sur
  l'alimentation du Linky). Nouvelles commandes :
  `soft-rx-stats` (statistiques et retard mesuré),
  `set-rx-skew <µs>` (retard imposé, `0` = calibration automatique),
  `set-std-labels <0|1>`.
- **Zigbee** : les mises à jour d'attributs prennent désormais le verrou de la pile Zigbee
  (il manquait : accès concurrent possible avec une lecture du coordinateur).
- **Numérotation au-dessus de toute version officielle** (`V3.3.0-std` pour la première
  version, `V3.4.10-std` aujourd'hui ; dernière version officielle : V3.2.13) : ni la page de
  mise à jour GammaTroniques ni un coordinateur Zigbee ne proposeront de « mise à jour »
  qui écraserait la correction. Le champ Zigbee *Date code* se termine par `-std`.
- Compilation : `htmlmin2` au lieu de `htmlmin`, qui ne s'installe plus en Python 3.13.

### Depuis la V3.3.0

Les points ci-dessus décrivent la première version (`V3.3.0-std`). Les versions suivantes,
jusqu'à la `V3.4.10-std`, ont ajouté :

- **Bascule heures pleines / heures creuses remontée en quelques secondes** (V3.4.7) : le
  firmware décode le profil du lendemain envoyé par le Linky (`PJOURF+1`), se réveille 30 s
  avant chaque changement de tarif et envoie aussitôt le nouveau tarif. Dans Domoticz, la
  bascule apparaît en quelques secondes au lieu d'environ 1 min 15.
  Nouvelle commande console `bascule-info` (prochaines bascules, prochain réveil).
- **Valeurs abîmées sur la ligne TIC rejetées** (V3.4.7 et V3.4.10) : il arrive qu'un bit
  soit faux sur un caractère sans que la somme de contrôle TIC (6 bits) le voie. Un libellé
  de tarif pouvait arriver abîmé (« Unknown Tarif » dans Zigbee for Domoticz, environ une
  fois par jour), un index arriver tronqué et donc sembler baisser. Les lignes contenant un
  caractère de contrôle et les valeurs numériques contenant autre chose que des chiffres
  sont désormais rejetées, l'ancienne valeur étant conservée (modes standard et historique
  pour les valeurs numériques).
- **Valeurs fixes du compteur envoyées en Zigbee** (V3.4.10) : puissance souscrite,
  puissance de coupure, numéro de compteur, point de livraison, nom du calendrier tarifaire
  (`NGTF`, que le coordinateur ne recevait jamais)… Elles partent au démarrage puis à chaque
  changement, au lieu de n'être connues qu'à l'appairage (après un passage de 9 kVA à 6 kVA,
  l'ancienne valeur restait affichée).
- **Relève rapide après chaque envoi** (V3.4.10) : pendant 10 s, le TICMeter interroge son
  routeur en continu, et les demandes de lecture du coordinateur reçoivent une réponse en
  moins d'une seconde (environ une sur deux auparavant). Testé avec Zigbee for Domoticz,
  pas avec Zigbee2MQTT.
- **`soft-rx-stats`** (V3.3.1) : le score de calibration n'est plus remis à zéro à chaque
  lecture.
- **Fichier OTA Zigbee retiré des releases** : il n'a jamais été testé. La mise à jour se
  fait en USB, par la page web ou avec esptool.

Détail version par version : [releases GitHub](https://github.com/Bastien1307/TICMeter-by-Bastien1307/releases)
et [CHANGELOG.md](CHANGELOG.md).

### Installation

**Le plus simple : la page web** 👉 **https://bastien1307.github.io/TICMeter-by-Bastien1307/**
(Chrome, Edge ou Opera sur ordinateur). Branchez le TICMeter en USB, cliquez sur « Connecter » :
la page affiche son état, permet de **sauvegarder la flash**, de **mettre à jour** (configuration et
appairage Zigbee conservés) et de **régler** le TICMeter (mode de communication, mode TIC, Wi-Fi, MQTT…).

**En ligne de commande**, avec esptool :

Testé sur un TICMeter matériel 3.4.2 (ESP32-C6), Linky monophasé, mode Zigbee.

**Avant tout, sauvegarder la flash complète** (retour possible à l'identique) :

```bash
esptool.py --chip esp32c6 -p <PORT> read_flash 0 0x400000 sauvegarde_ticmeter.bin
```

**Flasher** les fichiers de la [release](../../releases) (la configuration et l'appairage
Zigbee sont conservés : la zone NVS n'est pas touchée) :

```bash
esptool.py --chip esp32c6 -p <PORT> -b 460800 write_flash \
  0x10000 ota_data_initial.bin \
  0x17000 storage.bin \
  0x30000 TICMeter.bin
```

`<PORT>` : par exemple `/dev/cu.usbmodem2101` (macOS), `/dev/ttyACM0` (Linux), `COM3` (Windows).
Ne pas laisser la page de mise à jour GammaTroniques ouverte : elle monopolise le port.

**Revenir au firmware officiel** : depuis la page de mise à jour GammaTroniques, ou en
réécrivant la sauvegarde (`write_flash 0 sauvegarde_ticmeter.bin`).

### Avec Domoticz (Zigbee for Domoticz)

En mode Zigbee, le TICMeter est reconnu par le plugin [Zigbee for Domoticz](https://github.com/zigbeefordomoticz/Domoticz-Zigbee) (Z4D). C'est la configuration sur laquelle cette version a été testée.

<img src="https://zigbeefordomoticz.github.io/wiki/Images/GammaTroniques-TICMeter.png" alt="TICMeter dans Domoticz avec Zigbee for Domoticz" height="600"/>

*Capture et intégration : [pipiche38](https://github.com/pipiche38), auteur de Zigbee for Domoticz, qui l'avait proposée au dépôt d'origine ([PR #36](https://github.com/GammaTroniques/TICMeter/pull/36), jamais fusionnée).*

La capture montre les widgets créés à l'appairage pour un Linky en **mode standard, monophasé, contrat heures pleines / heures creuses** :

- en **orange**, les widgets propres aux contrats **Tempo ou EJP** (couleur du jour, couleur du lendemain, index des jours bleus et rouges) : inutiles avec un autre contrat, on peut les masquer ;
- en **bleu**, les widgets du **triphasé** (tension et puissance par phase) : utilisés en plus des autres sur une installation triphasée.

Pour aller plus loin, sur le wiki de Zigbee for Domoticz (en anglais) :

- [la page TICMeter](https://zigbeefordomoticz.github.io/wiki/en-eng/Corner_GammaTroniques.html) : la liste complète des widgets et, pour chacun, les modes où il fonctionne (historique ou standard, monophasé ou triphasé), ainsi qu'un second exemple en mode historique, contrat Base ;
- [la page technique](https://github.com/zigbeefordomoticz/wiki/blob/master/en-eng/Technical/ticmeter-integration.md) : la correspondance entre les champs du Linky et les attributs Zigbee (par exemple `EASF01` → index 1, `EASF02` → index 2) ;
- toutes les données reçues du TICMeter sont aussi visibles dans l'interface du plugin : **WebUI → Manufacturer → GammaTroniques**.

### Compiler

ESP-IDF **v5.2.1**, cible `esp32c6`. Appliquer à ESP-IDF la seule modification utile du
correctif fourni dans `firmware/patch/` : dans `components/esp_hw_support/sleep_modes.c`,
remplacer `CONFIG_ESP_CONSOLE_UART_BAUDRATE` par `115200` dans `UART_FLUSH_US_PER_CHAR`
(le fichier fourni vient d'une autre version d'ESP-IDF et ne compile pas tel quel).

```bash
cd firmware
idf.py build
```

### Soutenir

Ce firmware est gratuit et complet, sans rien de réservé. Si ce travail vous est utile,
vous pouvez m'offrir un café, **si vous le souhaitez** :

**[☕ paypal.me/sebastienRanc](https://paypal.me/sebastienRanc)**

> Choisir l'option **« Entre proches »** (plutôt que « Biens et services ») pour que le don
> arrive sans frais.

Et une pensée pour **[GammaTroniques](https://github.com/GammaTroniques/TICMeter)**,
qui a conçu le TICMeter et publié son firmware.

---

## 🇬🇧 English

### What this fixes

With a Linky meter in **standard TIC mode** (9600 baud), some TICMeters read heavily
corrupted frames: over a thousand checksum errors per reading, only about ten fields
decoded out of forty, contract "UNKNOWN", indexes losing a digit, missing Zigbee attributes.
**Historical mode** (1200 baud) works fine.

**Cause**: the input demodulator stage releases late. Every high level arrives about 40 µs
short, and the neighbouring low level as much longer. Harmless at 1200 baud, but at
9600 baud (104 µs per bit) the ESP32 UART, which samples mid-bit, reads some 1s as 0s.
The distortion is regular: nothing is lost.

**Fix**: in standard mode, a **software receiver** replaces the UART. It timestamps every
edge, **measures the delay itself** at the start of each reading (automatic calibration,
0–60 µs), compensates it, rebuilds the 7E1 bytes and hands them to the original decoder.
It only runs during reading windows. Historical mode still uses the UART.

**Measured result**: from ~1,300 checksum errors per reading down to **0–3**, 41 fields
instead of 11, contract recognised, peak / off-peak switching reported to Domoticz.

**Other changes**: standard-mode labels trimmed (`  HEURE  PLEINE  ` → `HEURE PLEINE`,
configurable with `set-std-labels 1`); console available in Zigbee mode when USB is
plugged (`soft-rx-stats`, `set-rx-skew`, `set-std-labels`); Zigbee attribute updates now
take the Zigbee stack lock; version numbers above any official release (`V3.3.0-std` for the
first build, `V3.4.10-std` today; latest official release: V3.2.13), so neither the
GammaTroniques update page nor a Zigbee coordinator will offer an "update" that would
overwrite the fix; `htmlmin2` build dependency.

**Since V3.3.0** (the points above describe the first build, `V3.3.0-std`):

- **Peak / off-peak switching reported within seconds** (V3.4.7): the firmware decodes the
  next-day profile sent by the Linky (`PJOURF+1`), wakes up 30 s before each tariff change
  and sends the new tariff immediately (a few seconds in Domoticz instead of about 1 min 15).
  New console command `bascule-info` (upcoming tariff changes, next wake-up).
- **Corrupted values on the TIC line discarded** (V3.4.7 and V3.4.10): a single wrong bit in
  a character can go unseen by the 6-bit TIC checksum. A tariff label could arrive corrupted
  ("Unknown Tarif" in Zigbee for Domoticz, about once a day), an index could arrive
  truncated and so appear to go down. Lines containing a control character and numeric
  values containing anything but digits are now discarded, the previous value being kept
  (standard and historical modes for numeric values).
- **Static meter values reported over Zigbee** (V3.4.10): subscribed power, cut-off power,
  meter serial number, delivery point, tariff calendar name (`NGTF`, which the coordinator
  never received)… They are sent at startup and whenever they change, instead of only being
  known at pairing (after going from 9 kVA to 6 kVA, the old value stayed displayed).
- **Fast polling after each report** (V3.4.10): for 10 s the TICMeter polls its router
  continuously, and read requests from the coordinator are answered within a second (about
  one in two before). Tested with Zigbee for Domoticz, not with Zigbee2MQTT.
- **`soft-rx-stats`** (V3.3.1): the calibration score is no longer reset after each reading.
- **Zigbee OTA file removed from the releases**: it was never tested. Updating is done over
  USB, with the web page or esptool.

Details for each version: [GitHub releases](https://github.com/Bastien1307/TICMeter-by-Bastien1307/releases)
and [CHANGELOG.md](CHANGELOG.md).

**Install**: easiest is the web page 👉 **https://bastien1307.github.io/TICMeter-by-Bastien1307/**
(Chrome, Edge or Opera on a computer): connect over USB, back up, update and configure the TICMeter
from the browser. Command line: back up the whole flash first
(`esptool.py --chip esp32c6 -p <PORT> read_flash 0 0x400000 backup.bin`), then flash the
[release](../../releases) files at `0x10000` (`ota_data_initial.bin`), `0x17000`
(`storage.bin`) and `0x30000` (`TICMeter.bin`). Configuration and Zigbee pairing are kept.
Tested on TICMeter hardware 3.4.2 (ESP32-C6), single-phase Linky, Zigbee mode.
Do not leave the GammaTroniques update page open: it holds the serial port.
**Back to the official firmware**: from the GammaTroniques update page, or by writing the
backup back (`write_flash 0 backup.bin`).

**Domoticz**: in Zigbee mode the TICMeter works with the [Zigbee for Domoticz](https://github.com/zigbeefordomoticz/Domoticz-Zigbee) plugin (the setup this version was tested on) — see the screenshot above, by [pipiche38](https://github.com/pipiche38), author of Zigbee for Domoticz ([PR #36](https://github.com/GammaTroniques/TICMeter/pull/36) on the original repository, never merged). The screenshot shows a standard-mode, single-phase, peak / off-peak setup: orange widgets are for Tempo / EJP contracts only, blue ones for three-phase installations. Full widget list and technical mapping on the Zigbee for Domoticz wiki: [TICMeter page](https://zigbeefordomoticz.github.io/wiki/en-eng/Corner_GammaTroniques.html), [technical page](https://github.com/zigbeefordomoticz/wiki/blob/master/en-eng/Technical/ticmeter-integration.md).

**Build**: ESP-IDF **v5.2.1**, target `esp32c6`, then `idf.py build` in `firmware/`. Apply
to ESP-IDF the only useful change from the patch in `firmware/patch/`: in
`components/esp_hw_support/sleep_modes.c`, replace `CONFIG_ESP_CONSOLE_UART_BAUDRATE` with
`115200` in `UART_FLUSH_US_PER_CHAR` (the provided file comes from another ESP-IDF version
and does not build as is).

**Support**: this firmware is free and complete. If it helps you, you may buy me a coffee,
**only if you wish**: **[☕ paypal.me/sebastienRanc](https://paypal.me/sebastienRanc)**
(please pick "Friends and family").

---

## Licence / License

[CC BY-NC 4.0](LICENCE.md) — comme le projet d'origine / same as the original project.
Œuvre originale / Original work: © [GammaTroniques](https://github.com/GammaTroniques/TICMeter).
Modifications © Bastien1307, listées dans / listed in [CHANGELOG.md](CHANGELOG.md).
Usage commercial interdit / No commercial use.

README d'origine / Original README: [README.GammaTroniques.md](README.GammaTroniques.md).
