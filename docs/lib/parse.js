// Lecture des lignes envoyées par le TICMeter sur l'USB (réponses de la console et journaux).
// Fonctions pures : testables hors navigateur.

// Masque les secrets avant affichage (clé Tuya)
export function maskSecrets(line) {
  return line.replace(/^(Device Auth: ?)(\S+)/, (m, p, v) => p + "•".repeat(Math.min(v.length, 12)));
}

export function stripAnsi(line) {
  return line.replace(/\x1b\[[0-9;]*m/g, "").replace(/[\x00-\x08\x0b-\x1f]/g, "").trimEnd();
}

// Retire le préfixe des journaux ESP-IDF : "I (78296) LINKY: " → "LINKY: …"
function stripLogPrefix(line) {
  return line.replace(/^[IWEDV] \(\d+\) /, "");
}

const RULES = [
  // réponses de la console
  [/App version:\s*(\S+)/, (m, s) => { s.version = m[1]; }],
  [/^Mode: (\d+) - (\S+)/, (m, s) => { s.mode = +m[1]; }],
  [/Configured Linky mode: (\d+)/, (m, s) => { s.linkyMode = +m[1]; }],
  [/Current Linky mode: \d+: (\S+)/, (m, s) => { s.linkyCurrent = m[1]; }],
  [/^Refresh: (\d+)/, (m, s) => { s.refresh = +m[1]; }],
  [/^Soft RX: (active|inactive)/, (m, s) => { s.softRx = m[1] === "active"; }],
  [/^Skew: (\d+) us \((auto|forced)\)/, (m, s) => { s.skew = +m[1]; s.skewForced = m[2] === "forced"; }],
  [/^Last calibration score: (\d+)\/1000/, (m, s) => { s.calScore = +m[1]; }],
  [/^Bytes: (\d+), parity errors: (\d+), frame errors: (\d+)/, (m, s) => {
    s.rxBytes = +m[1]; s.rxParity = +m[2]; s.rxFrame = +m[3];
  }],
  [/^STD labels: (raw|cleaned)/, (m, s) => { s.rawLabels = m[1] === "raw"; }],
  [/^SSID: (.*)$/, (m, s) => { s.ssid = m[1]; }],
  // clés Tuya : on ne garde que leur présence, jamais leur valeur
  [/^Device UUID: ?(.*)$/, (m, s) => { s.tuyaUuid = m[1].trim().length > 0; }],
  [/^Device Auth: ?(.*)$/, (m, s) => { s.tuyaAuth = m[1].trim().length > 0; }],
  [/^Host: (.*)$/, (m, s) => { s.mqttHost = m[1]; }],
  [/^Port: (\d+)/, (m, s) => { s.mqttPort = +m[1]; }],
  [/^Topic: (.*)$/, (m, s) => { s.mqttTopic = m[1]; }],
  [/^Username: (.*)$/, (m, s) => { s.mqttUser = m[1]; }],
  // journaux de lecture du Linky
  [/^LINKY: Linky contract: (.+)$/, (m, s) => { s.contract = m[1].trim(); }],
  [/^LINKY: Linky checksum error: (\d+)/, (m, s) => { s.checksumErrors = +m[1]; s.lastReading = new Date(); }],
  [/^LINKY: Linky decode count: (\d+)/, (m, s) => { s.fields = +m[1]; }],
  [/^LINKY: Linky mode: (\S+)/, (m, s) => { s.linkyCurrent = m[1]; }],
  [/^SOFT_RX: Calibration : retard (\d+) us .*score (\d+)/, (m, s) => { s.skew = +m[1]; s.calScore = +m[2]; }],
  [/^MAIN: Data sent/, (m, s) => { s.lastSend = "ok"; }],
  [/^MAIN: .*send failed/i, (m, s) => { s.lastSend = "failed"; }],
  [/^ZIGBEE: File version: ([0-9a-f]+)/, (m, s) => { s.zigbeeFileVersion = m[1]; }],
  [/^cpu_start: App version:\s*(\S+)/, (m, s) => { s.version = m[1]; }],
];

// Met à jour l'état à partir d'une ligne ; renvoie true si la ligne a été reconnue.
export function parseLine(raw, state) {
  const line = stripLogPrefix(stripAnsi(raw)).trim();
  for (const [re, apply] of RULES) {
    const m = line.match(re);
    if (m) {
      apply(m, state);
      return true;
    }
  }
  return false;
}

// "V3.3.0-std" → [3, 3, 0] ; null si illisible
export function parseVersion(v) {
  const m = /^v?(\d+)\.(\d+)\.(\d+)/i.exec(v || "");
  return m ? [+m[1], +m[2], +m[3]] : null;
}

// < 0 si a est plus ancienne que b, 0 si égales, > 0 si plus récente
export function compareVersions(a, b) {
  const x = parseVersion(a), y = parseVersion(b);
  if (!x || !y) return NaN;
  for (let i = 0; i < 3; i++) if (x[i] !== y[i]) return x[i] - y[i];
  return 0;
}

// Protège un argument de la console (esp_console découpe sur les espaces, accepte les guillemets)
export function quoteArg(v) {
  const s = String(v);
  if (s === "" || /[\s"\\]/.test(s)) return '"' + s.replace(/\\/g, "\\\\").replace(/"/g, '\\"') + '"';
  return s;
}
