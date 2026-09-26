# TICMeter by Bastien1307 (non officiel / unofficial)

[![licence](https://img.shields.io/badge/License-CC_BY--NC_4.0-lightgrey.svg?style=for-the-badge)](https://creativecommons.org/licenses/by-nc/4.0/)

> **Version non officielle** du firmware du [TICMeter de GammaTroniques](https://github.com/GammaTroniques/TICMeter).
> GammaTroniques n'est ni l'auteur ni responsable de ces modifications et n'apporte aucun support sur cette version.
>
> *Unofficial build of the [GammaTroniques TICMeter](https://github.com/GammaTroniques/TICMeter) firmware.
> GammaTroniques is neither the author of these changes nor responsible for them. English below.*

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

*Plug the TICMeter over USB, open the page in Chrome, Edge or Opera (desktop) and click
"Connecter": status, flash backup / restore, update (settings and Zigbee pairing kept),
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
- **Version 3.3.0** (`V3.3.0-std`), au-dessus de toute version officielle : ni la page de
  mise à jour GammaTroniques ni un coordinateur Zigbee ne proposeront de « mise à jour »
  qui écraserait la correction. Le champ Zigbee *Date code* se termine par `-std`.
- Compilation : `htmlmin2` au lieu de `htmlmin`, qui ne s'installe plus en Python 3.13.

Détail : [CHANGELOG.md](CHANGELOG.md).

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

**Mise à jour par Zigbee** : le fichier `TICMeter.ota` de la release est prévu pour une
mise à jour sans fil depuis le coordinateur. **Non testé à ce jour.**

**Revenir au firmware officiel** : depuis la page de mise à jour GammaTroniques, ou en
réécrivant la sauvegarde (`write_flash 0 sauvegarde_ticmeter.bin`).

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
take the Zigbee stack lock; version **3.3.0** (`V3.3.0-std`); `htmlmin2` build dependency.
See [CHANGELOG.md](CHANGELOG.md).

**Install**: easiest is the web page 👉 **https://bastien1307.github.io/TICMeter-by-Bastien1307/**
(Chrome, Edge or Opera on a computer): connect over USB, back up, update and configure the TICMeter
from the browser. Command line: back up the whole flash first
(`esptool.py --chip esp32c6 -p <PORT> read_flash 0 0x400000 backup.bin`), then flash the
[release](../../releases) files at `0x10000` (`ota_data_initial.bin`), `0x17000`
(`storage.bin`) and `0x30000` (`TICMeter.bin`). Configuration and Zigbee pairing are kept.
Zigbee OTA file provided but **not tested yet**.

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
