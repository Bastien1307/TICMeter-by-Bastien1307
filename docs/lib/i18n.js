// Traduction de la page : le français est la langue d'origine (texte du HTML),
// l'anglais est appliqué par-dessus en remplaçant les textes fixes, puis restauré au retour en français.

// Textes fixes du HTML (espaces normalisés) → anglais
const EN = {
  "Connecter, mettre à jour et paramétrer votre TICMeter par USB. Firmware": "Connect, update and configure your TICMeter over USB. Firmware",
  ": version": ":",
  "non officielle": "unofficial",
  "du firmware de": "version of the firmware by",
  "code source et explications": "source code and explanations",
  "Votre navigateur ne sait pas communiquer avec un port USB. Utilisez": "Your browser cannot talk to a USB port. Use",
  "ou": "or",
  "et": "and",
  "sur ordinateur (Windows, macOS, Linux, ChromeOS). Safari, Firefox, Brave et les téléphones ne sont pas compatibles.":
    "on a computer (Windows, macOS, Linux, ChromeOS). Safari, Firefox, Brave and phones are not supported.",
  "TICMeter non connecté": "TICMeter not connected",
  "Connecter le TICMeter": "Connect the TICMeter",
  "Redémarrer le TICMeter": "Restart the TICMeter",
  "Déconnecter le TICMeter": "Disconnect the TICMeter",
  "1. Connexion": "1. Connection",
  "Branchez le TICMeter à l'ordinateur avec un câble USB-C (il peut rester raccordé au Linky), puis cliquez sur « Connecter ». Choisissez le port nommé":
    "Plug the TICMeter into the computer with a USB-C cable (it can stay wired to the Linky), then click \"Connect\". Choose the port named",
  "Non connecté": "Not connected",
  "Navigateurs compatibles :": "Supported browsers:",
  ", sur ordinateur (Windows, macOS, Linux, ChromeOS). Non compatibles : Safari, Firefox, Brave, et les téléphones ou tablettes.":
    ", on a computer (Windows, macOS, Linux, ChromeOS). Not supported: Safari, Firefox, Brave, phones and tablets.",
  "Le TICMeter ne répond pas aux commandes. En mode Zigbee, la console ne démarre que si l'USB est branché":
    "The TICMeter does not answer commands. In Zigbee mode, the console only starts if USB is plugged in",
  "au démarrage": "at startup",
  ", et le firmware officiel ne l'a pas du tout en Zigbee.": ", and the official firmware has no console at all in Zigbee mode.",
  "(environ 1 minute). La mise à jour reste possible sans console.": "(about 1 minute). Updating still works without the console.",
  "2. État du TICMeter": "2. TICMeter status",
  "Actualiser": "Refresh",
  "Mode TIC": "TIC mode",
  "Contrat": "Contract",
  "Dernière lecture": "Last reading",
  "Envoi": "Sending",
  "Récepteur mode standard": "Standard-mode receiver",
  "Retard compensé": "Compensated delay",
  "3. Mise à jour": "3. Update",
  "Recherche de la dernière version…": "Looking for the latest version…",
  "Sauvegarder la flash (4 Mo)": "Back up the flash (4 MB)",
  "Mettre à jour": "Update",
  "La mise à jour garde la configuration et l'appairage Zigbee. La sauvegarde permet de revenir exactement à l'état actuel (fichier à conserver précieusement).":
    "Updating keeps the settings and the Zigbee pairing. The backup lets you go back exactly to the current state (keep that file safe).",
  "Restaurer une sauvegarde": "Restore a backup",
  "Réécrit toute la flash avec un fichier de sauvegarde de 4 Mo fait sur": "Rewrites the whole flash with a 4 MB backup file made on",
  "ce": "this",
  "TICMeter.": "TICMeter.",
  "Restaurer": "Restore",
  "4. Paramètres": "4. Settings",
  "Chaque réglage est enregistré dans le TICMeter dès que vous cliquez sur « Enregistrer ». Redémarrez ensuite le TICMeter pour qu'il soit pris en compte.":
    "Each setting is stored in the TICMeter as soon as you click \"Save\". Then restart the TICMeter to apply it.",
  "Mode de communication": "Communication mode",
  "Enregistrer": "Save",
  "Mode TIC du Linky": "Linky TIC mode",
  "Automatique": "Automatic",
  "Historique": "Historical",
  "Intervalle entre deux envois (secondes, 30 minimum)": "Interval between two sends (seconds, 30 minimum)",
  "Libellés du mode standard (tarif, contrat)": "Standard-mode labels (tariff, contract)",
  "Nettoyés : « HEURE PLEINE » (recommandé)": "Cleaned: « HEURE PLEINE » (recommended)",
  "Texte brut du Linky : « HEURE PLEINE »": "Raw Linky text: «  HEURE  PLEINE  »",
  "Retard compensé du récepteur (µs) — laisser 0 pour la calibration automatique":
    "Receiver compensated delay (µs) — leave 0 for automatic calibration",
  "Ce firmware ne propose pas les réglages du mode standard : mettez-le à jour (section 3).":
    "This firmware has no standard-mode settings: update it (section 3).",
  "Wi-Fi (modes Web, MQTT et Tuya)": "Wi-Fi (Web, MQTT and Tuya modes)",
  "Nom du réseau (SSID)": "Network name (SSID)",
  "Mot de passe": "Password",
  "Enregistrer le Wi-Fi": "Save Wi-Fi",
  "Serveur MQTT": "MQTT server",
  "Adresse": "Address",
  "Utilisateur": "User",
  "Enregistrer le MQTT": "Save MQTT",
  "Console et journal du TICMeter": "TICMeter console and log",
  "Copier le journal": "Copy the log",
  "Enregistrer dans un fichier": "Save to a file",
  "Effacer": "Clear",
  "Envoyer": "Send",
  "Ce firmware vous rend service ?": "Is this firmware helping you?",
  "Offrez-moi un café !": "Buy me a coffee!",
  "Firmware TICMeter by Bastien1307 et cette page sous licence": "TICMeter by Bastien1307 firmware and this page licensed under",
  ". TICMeter est un produit": ". TICMeter is a product by",
  ", qui n'est pas responsable de cette version. Mise à jour par": ", who is not responsible for this version. Updates by",
};

// Attributs (placeholder, title)
const EN_ATTR = {
  "Commande (ex. help, linky-print 0, soft-rx-stats)": "Command (e.g. help, linky-print 0, soft-rx-stats)",
  "Ce firmware est gratuit : un café si vous le souhaitez (choisir « Entre proches » sur PayPal)":
    "This firmware is free: a coffee if you wish (pick \"Friends and family\" on PayPal)",
};

const norm = (t) => t.replace(/\s+/g, " ").trim();
const original = new WeakMap(); // nœud → texte français d'origine
const originalAttr = new WeakMap();

// Parcourt les textes de la page, sauf le journal du TICMeter
function* textNodes(root) {
  const walker = document.createTreeWalker(root, NodeFilter.SHOW_TEXT, {
    acceptNode: (n) => (n.parentElement.closest("#log, script, style") ? NodeFilter.FILTER_REJECT : NodeFilter.FILTER_ACCEPT),
  });
  let n;
  while ((n = walker.nextNode())) yield n;
}

export function applyLang(lang, root = document.body) {
  document.documentElement.lang = lang;
  for (const n of textNodes(root)) {
    if (!original.has(n)) {
      if (!EN[norm(n.nodeValue)]) continue;
      original.set(n, n.nodeValue);
    }
    const fr = original.get(n);
    if (lang === "en") {
      const lead = fr.match(/^\s*/)[0], trail = fr.match(/\s*$/)[0];
      n.nodeValue = lead + EN[norm(fr)] + trail;
    } else {
      n.nodeValue = fr;
    }
  }
  for (const el of root.querySelectorAll("[placeholder], [title]")) {
    for (const attr of ["placeholder", "title"]) {
      const v = el.getAttribute(attr);
      if (v == null) continue;
      let saved = originalAttr.get(el) || {};
      if (!(attr in saved)) {
        if (!EN_ATTR[v]) continue;
        saved[attr] = v;
        originalAttr.set(el, saved);
      }
      el.setAttribute(attr, lang === "en" ? EN_ATTR[saved[attr]] : saved[attr]);
    }
  }
}
