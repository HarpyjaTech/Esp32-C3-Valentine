/*
 * CAIXA DE PRESENTE "SWIM" - ESP32-S3
 * Caixa de madeira + Trava de metal + Buzzer Passivo + Fita LED RGB + Web Server
 * Melodia: "Swim" - BTS (arranjo simplificado para buzzer)
 *
 * Funcionamento:
 *  1. A trava de metal funciona como um interruptor: fechada = contato fechado.
 *  2. Ao destravar a caixa, o ESP32-S3 espera o atraso configurado (padrão 500 ms).
 *  3. O buzzer passivo toca a melodia, com volume (dinâmica) variando nota a nota.
 *  4. A fita LED acompanha o volume do buzzer em tempo real:
 *       volume baixo -> VERMELHO ... volume alto -> VIOLETA
 *  5. O atraso, o volume máximo, o brilho da fita e o liga/desliga da cena
 *     podem ser alterados pelo web server (http://caixa.local ou 192.168.4.1).
 *
 * Placa:  ESP32S3 Dev Module (Arduino-ESP32 core 3.x)
 * Libs:   Adafruit NeoPixel (somente se FITA_ENDERECAVEL = 1)
 */

#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Preferences.h>

// ============ TIPO DA FITA LED =============
// 1 = fita endereçável (WS2812B / SK6812, 5V, fio único de dados)
// 0 = fita RGB analógica comum (5050, 12V, fios +12V/R/G/B) acionada por 3 MOSFETs
#define FITA_ENDERECAVEL 1

#if FITA_ENDERECAVEL
#include <Adafruit_NeoPixel.h>
#endif

// ============ PINOS =============
#define PIN_TRAVA       5    // Trava de metal (contato) -> GND quando fechada
#define PIN_BUZZER      4    // Buzzer passivo (PWM)
#define PIN_FITA_DADOS  6    // DIN da fita endereçável
#define NUM_LEDS        60   // Quantidade de LEDs da fita endereçável
#define PIN_FITA_R      15   // Gate do MOSFET do vermelho (fita analógica)
#define PIN_FITA_G      16   // Gate do MOSFET do verde    (fita analógica)
#define PIN_FITA_B      17   // Gate do MOSFET do azul     (fita analógica)

// Com INPUT_PULLUP: trava fechada encosta no contato ligado ao GND -> LOW.
// Se usar um sensor que funcione ao contrário, troque para HIGH.
#define TRAVA_FECHADA_NIVEL LOW

// ============ WI-FI =============
// Deixe WIFI_SSID vazio para a caixa criar a própria rede (modo Access Point).
const char* WIFI_SSID  = "";
const char* WIFI_SENHA = "";
const char* AP_SSID    = "Caixa-Swim";
const char* AP_SENHA   = "swim2026";   // mínimo de 8 caracteres
const char* MDNS_NOME  = "caixa";      // http://caixa.local

// ============ CONFIGURAÇÕES PADRÃO =============
#define ATRASO_PADRAO_MS   500   // Atraso entre destravar e iniciar a cena
#define ATRASO_MAX_MS      10000
#define VOLUME_PADRAO      100   // Volume máximo do buzzer (%)
#define BRILHO_PADRAO      180   // Brilho da fita (0-255)
#define DEBOUNCE_MS        40    // Filtro de ruído do contato da trava
#define INTERVALO_LED_MS   10    // Atualização da fita/buzzer durante a cena
#define HUE_VIOLETA        275.0f // Matiz do violeta (0 = vermelho)
#define BUZZER_RES_BITS    10    // Resolução do PWM do buzzer

// ============ MELODIA =============
#define REST 0
#define NOTE_G4  392
#define NOTE_A4  440
#define NOTE_B4  494
#define NOTE_C5  523
#define NOTE_D5  587
#define NOTE_E5  659
#define NOTE_F5  698
#define NOTE_G5  784
#define NOTE_A5  880
#define NOTE_B5  988
#define NOTE_C6  1047
#define NOTE_D6  1175

// Andamento: duração das notas em semicolcheias (1 = 1/16 de compasso)
#define BPM 96
const unsigned long MS_SEMICOLCHEIA = 60000UL / BPM / 4;

struct Nota {
  uint16_t freq;  // Hz (REST = pausa)
  uint8_t  dur;   // em semicolcheias
  uint8_t  vol;   // dinâmica da nota, 0-100 (define a cor da fita)
};

// Arranjo simplificado de "Swim" - BTS para buzzer monofônico.
// Confira de ouvido e ajuste notas/durações/volumes à vontade:
// cada linha é {frequência, duração, volume}.
const Nota melodia[] = {
  // Verso - suave
  {NOTE_E5, 2, 35}, {NOTE_E5, 2, 38}, {NOTE_G5, 2, 40}, {NOTE_A5, 4, 45},
  {NOTE_G5, 2, 40}, {NOTE_E5, 2, 38}, {NOTE_D5, 4, 35}, {REST,    2,  0},
  {NOTE_C5, 2, 35}, {NOTE_D5, 2, 38}, {NOTE_E5, 2, 42}, {NOTE_G5, 4, 48},
  {NOTE_E5, 2, 42}, {NOTE_D5, 6, 38}, {REST,    2,  0},

  // Pré-refrão - crescendo
  {NOTE_A4, 2, 50}, {NOTE_C5, 2, 55}, {NOTE_D5, 2, 60}, {NOTE_E5, 2, 65},
  {NOTE_G5, 2, 70}, {NOTE_A5, 2, 75}, {NOTE_B5, 4, 80}, {REST,    2,  0},

  // Refrão - forte
  {NOTE_C6, 4, 100}, {NOTE_B5, 2, 90}, {NOTE_A5, 2, 85}, {NOTE_G5, 4, 95},
  {NOTE_E5, 2, 80},  {NOTE_G5, 2, 85}, {NOTE_A5, 6, 100}, {REST,   2,  0},
  {NOTE_A5, 2, 90},  {NOTE_G5, 2, 85}, {NOTE_E5, 2, 80}, {NOTE_D5, 2, 75},
  {NOTE_E5, 4, 90},  {NOTE_G5, 4, 95}, {NOTE_C6, 8, 100}, {REST,   2,  0},

  // Final - fade-out
  {NOTE_G5, 4, 60}, {NOTE_E5, 4, 45}, {NOTE_D5, 4, 30}, {NOTE_C5, 8, 20},
};
const int TOTAL_NOTAS = sizeof(melodia) / sizeof(melodia[0]);

// ============ ESTADO =============
enum Estado { AGUARDANDO, CONTANDO_ATRASO, TOCANDO };
Estado estado = AGUARDANDO;

struct Config {
  uint16_t atrasoMs;
  uint8_t  volume;   // 0-100 %
  uint8_t  brilho;   // 0-255
  bool     ativo;    // cena habilitada ao destravar
} config;

Preferences prefs;
WebServer server(80);

#if FITA_ENDERECAVEL
Adafruit_NeoPixel fita(NUM_LEDS, PIN_FITA_DADOS, NEO_GRB + NEO_KHZ800);
#endif

bool travaFechada = true;           // estado filtrado
bool leituraAnterior = true;
unsigned long ultimaMudancaLeitura = 0;

unsigned long instanteDestrave = 0;
unsigned long inicioNota = 0;
unsigned long ultimaAtualizacao = 0;
int indiceNota = 0;
uint16_t freqAtual = 0;
float volumeLed = 0;                // volume suavizado usado na cor (0-1)

// ============ PROTÓTIPOS =============
void carregarConfig();
void salvarConfig();
void iniciarWiFi();
void configurarRotas();
void atualizarTrava();
void iniciarContagem();
void iniciarCena();
void atualizarCena();
void pararCena();
void buzzerTocar(uint16_t freq, float volume);
void fitaVolume(float volume);
void fitaCor(uint8_t r, uint8_t g, uint8_t b);
void fitaApagar();
void hsvParaRgb(float h, float s, float v, uint8_t &r, uint8_t &g, uint8_t &b);
float envelope(unsigned long t, unsigned long dur);
bool lerTravaFechada();

// =====================================================================
void setup() {
  Serial.begin(115200);
  delay(200);

  pinMode(PIN_TRAVA, INPUT_PULLUP);

  // Buzzer: PWM com duty variável (volume) e frequência variável (nota)
  ledcAttach(PIN_BUZZER, 2000, BUZZER_RES_BITS);
  ledcWrite(PIN_BUZZER, 0);

#if FITA_ENDERECAVEL
  fita.begin();
  fita.show();
#else
  ledcAttach(PIN_FITA_R, 5000, 8);
  ledcAttach(PIN_FITA_G, 5000, 8);
  ledcAttach(PIN_FITA_B, 5000, 8);
#endif
  fitaApagar();

  carregarConfig();

  travaFechada = lerTravaFechada();
  leituraAnterior = travaFechada;

  iniciarWiFi();
  configurarRotas();
  server.begin();

  Serial.println("Caixa Swim iniciada!");
  Serial.printf("Atraso: %u ms | Volume: %u%% | Brilho: %u | Ativo: %s\n",
                config.atrasoMs, config.volume, config.brilho, config.ativo ? "sim" : "nao");
  Serial.printf("Trava: %s\n", travaFechada ? "fechada" : "aberta");
}

void loop() {
  server.handleClient();
  atualizarTrava();

  switch (estado) {
    case CONTANDO_ATRASO:
      if (millis() - instanteDestrave >= config.atrasoMs) {
        iniciarCena();
      }
      break;
    case TOCANDO:
      atualizarCena();
      break;
    default:
      break;
  }
}

// ============ TRAVA =============
bool lerTravaFechada() {
  return digitalRead(PIN_TRAVA) == TRAVA_FECHADA_NIVEL;
}

void atualizarTrava() {
  bool leitura = lerTravaFechada();
  if (leitura != leituraAnterior) {
    leituraAnterior = leitura;
    ultimaMudancaLeitura = millis();
  }
  if (leitura == travaFechada || millis() - ultimaMudancaLeitura < DEBOUNCE_MS) {
    return;
  }

  travaFechada = leitura;
  if (!travaFechada) {
    Serial.println(">>> CAIXA DESTRAVADA! <<<");
    if (config.ativo && estado == AGUARDANDO) {
      iniciarContagem();
    }
  } else {
    Serial.println(">>> Caixa travada novamente <<<");
    pararCena();
  }
}

// ============ CENA =============
void iniciarContagem() {
  instanteDestrave = millis();
  estado = CONTANDO_ATRASO;
  Serial.printf("Iniciando cena em %u ms...\n", config.atrasoMs);
}

void iniciarCena() {
  indiceNota = 0;
  inicioNota = millis();
  ultimaAtualizacao = 0;
  volumeLed = 0;
  freqAtual = 0;
  estado = TOCANDO;
  Serial.println("Tocando Swim!");
}

void atualizarCena() {
  unsigned long agora = millis();
  unsigned long durNota = melodia[indiceNota].dur * MS_SEMICOLCHEIA;

  // Avança para a próxima nota
  while (agora - inicioNota >= durNota) {
    inicioNota += durNota;
    indiceNota++;
    if (indiceNota >= TOTAL_NOTAS) {
      pararCena();
      Serial.println("Cena finalizada!");
      return;
    }
    durNota = melodia[indiceNota].dur * MS_SEMICOLCHEIA;
  }

  if (agora - ultimaAtualizacao < INTERVALO_LED_MS) {
    return;
  }
  ultimaAtualizacao = agora;

  const Nota &n = melodia[indiceNota];
  float env = (n.freq == REST) ? 0.0f : envelope(agora - inicioNota, durNota);
  float volumeNota = (n.vol / 100.0f) * env;   // 0-1, independe do volume máximo

  buzzerTocar(n.freq, volumeNota * (config.volume / 100.0f));

  // Suaviza a cor para não "piscar" entre notas
  volumeLed += (volumeNota - volumeLed) * 0.35f;
  fitaVolume(volumeLed);
}

void pararCena() {
  if (estado != AGUARDANDO) {
    Serial.println("Cena interrompida.");
  }
  estado = AGUARDANDO;
  buzzerTocar(REST, 0);
  fitaApagar();
}

// Envelope de cada nota: ataque rápido, leve decaimento e soltura no final.
// Separa as notas e faz o volume (e a cor) "respirar" durante a música.
float envelope(unsigned long t, unsigned long dur) {
  unsigned long ataque  = min(30UL, dur / 4);
  unsigned long soltura = min(40UL, dur / 5);
  if (t >= dur) return 0.0f;
  if (ataque > 0 && t < ataque) return (float)t / ataque;
  if (soltura > 0 && t > dur - soltura) return (float)(dur - t) / soltura * 0.75f;
  float p = (float)(t - ataque) / (float)(dur - ataque - soltura + 1);
  return 1.0f - 0.25f * p;   // 100% -> 75%
}

// ============ BUZZER =============
// O volume do buzzer passivo é controlado pelo duty cycle do PWM:
// 0% = mudo ... 50% = volume máximo.
void buzzerTocar(uint16_t freq, float volume) {
  if (freq == REST || volume <= 0.0f) {
    ledcWrite(PIN_BUZZER, 0);
    return;
  }
  if (freq != freqAtual) {
    ledcChangeFrequency(PIN_BUZZER, freq, BUZZER_RES_BITS);
    freqAtual = freq;
  }
  const uint32_t dutyMax = 1UL << (BUZZER_RES_BITS - 1);   // 50%
  uint32_t duty = (uint32_t)(constrain(volume, 0.0f, 1.0f) * dutyMax);
  ledcWrite(PIN_BUZZER, duty);
}

// ============ FITA LED =============
// volume 0 -> vermelho (0°) ... volume 1 -> violeta (275°)
void fitaVolume(float volume) {
  volume = constrain(volume, 0.0f, 1.0f);
  uint8_t r, g, b;
  hsvParaRgb(volume * HUE_VIOLETA, 1.0f, config.brilho / 255.0f, r, g, b);
  fitaCor(r, g, b);
}

void fitaCor(uint8_t r, uint8_t g, uint8_t b) {
#if FITA_ENDERECAVEL
  fita.fill(fita.Color(r, g, b));
  fita.show();
#else
  ledcWrite(PIN_FITA_R, r);
  ledcWrite(PIN_FITA_G, g);
  ledcWrite(PIN_FITA_B, b);
#endif
}

void fitaApagar() {
  fitaCor(0, 0, 0);
}

void hsvParaRgb(float h, float s, float v, uint8_t &r, uint8_t &g, uint8_t &b) {
  float c = v * s;
  float x = c * (1 - fabsf(fmodf(h / 60.0f, 2) - 1));
  float m = v - c;
  float rf, gf, bf;
  if      (h < 60)  { rf = c; gf = x; bf = 0; }
  else if (h < 120) { rf = x; gf = c; bf = 0; }
  else if (h < 180) { rf = 0; gf = c; bf = x; }
  else if (h < 240) { rf = 0; gf = x; bf = c; }
  else if (h < 300) { rf = x; gf = 0; bf = c; }
  else              { rf = c; gf = 0; bf = x; }
  r = (uint8_t)((rf + m) * 255);
  g = (uint8_t)((gf + m) * 255);
  b = (uint8_t)((bf + m) * 255);
}

// ============ CONFIGURAÇÃO (NVS) =============
void carregarConfig() {
  prefs.begin("caixa", true);
  config.atrasoMs = prefs.getUShort("atraso", ATRASO_PADRAO_MS);
  config.volume   = prefs.getUChar("volume", VOLUME_PADRAO);
  config.brilho   = prefs.getUChar("brilho", BRILHO_PADRAO);
  config.ativo    = prefs.getBool("ativo", true);
  prefs.end();
}

void salvarConfig() {
  prefs.begin("caixa", false);
  prefs.putUShort("atraso", config.atrasoMs);
  prefs.putUChar("volume", config.volume);
  prefs.putUChar("brilho", config.brilho);
  prefs.putBool("ativo", config.ativo);
  prefs.end();
}

// ============ WI-FI =============
void iniciarWiFi() {
  if (strlen(WIFI_SSID) > 0) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_SENHA);
    Serial.printf("Conectando a %s", WIFI_SSID);
    unsigned long inicio = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - inicio < 10000) {
      delay(250);
      Serial.print(".");
    }
    Serial.println();
    if (WiFi.status() == WL_CONNECTED) {
      Serial.printf("Conectado! Acesse http://%s\n", WiFi.localIP().toString().c_str());
    } else {
      Serial.println("Falha ao conectar; criando rede propria.");
    }
  }

  if (WiFi.status() != WL_CONNECTED) {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID, AP_SENHA);
    Serial.printf("Rede: %s | Senha: %s | Acesse http://%s\n",
                  AP_SSID, AP_SENHA, WiFi.softAPIP().toString().c_str());
  }

  if (MDNS.begin(MDNS_NOME)) {
    MDNS.addService("http", "tcp", 80);
    Serial.printf("Ou acesse http://%s.local\n", MDNS_NOME);
  }
}

// ============ WEB SERVER =============
const char PAGINA[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html lang="pt-BR"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Caixa Swim</title>
<style>
body{font-family:sans-serif;max-width:420px;margin:0 auto;padding:16px;background:#1b1030;color:#f3eaff}
h1{font-size:1.4em;background:linear-gradient(90deg,#ff2a2a,#ff9a00,#ffe600,#2ee66b,#2bb8ff,#8b3dff);-webkit-background-clip:text;background-clip:text;color:transparent}
.card{background:#2a1a48;border-radius:12px;padding:16px;margin-bottom:14px}
label{display:block;margin-top:12px}
input[type=number],input[type=range]{width:100%;box-sizing:border-box}
input[type=number]{padding:8px;border-radius:8px;border:0;font-size:1em}
button{width:100%;padding:12px;margin-top:12px;border:0;border-radius:8px;font-size:1em;cursor:pointer}
.p{background:#8b3dff;color:#fff}.s{background:#46356a;color:#fff}
.bar{height:10px;border-radius:5px;background:linear-gradient(90deg,red,orange,yellow,lime,cyan,blue,#8b00ff)}
small{opacity:.7}
</style></head><body>
<h1>🎁 Caixa Swim</h1>
<div class="card">
<b>Trava:</b> <span id="trava">-</span><br>
<b>Cena:</b> <span id="estado">-</span>
</div>
<form class="card" method="POST" action="/salvar">
<label>Atraso após destravar (ms)
<input type="number" name="atraso" min="0" max="%ATRASO_MAX%" step="50" value="%ATRASO%"></label>
<label>Volume máximo do buzzer: <span id="vv">%VOLUME%</span>%
<input type="range" name="volume" min="0" max="100" value="%VOLUME%" oninput="vv.textContent=this.value"></label>
<label>Brilho da fita LED: <span id="bv">%BRILHO%</span>
<input type="range" name="brilho" min="0" max="255" value="%BRILHO%" oninput="bv.textContent=this.value"></label>
<label><input type="checkbox" name="ativo" value="1" %ATIVO%> Tocar ao destravar a caixa</label>
<button class="p" type="submit">Salvar</button>
</form>
<div class="card">
<div class="bar"></div><small>Volume baixo = vermelho · volume alto = violeta</small>
<button class="s" onclick="fetch('/testar',{method:'POST'})">▶ Testar cena (com atraso)</button>
<button class="s" onclick="fetch('/parar',{method:'POST'})">■ Parar</button>
</div>
<script>
const nomes={aguardando:'aguardando',contando:'contando atraso…',tocando:'tocando 🎵'};
async function st(){try{const r=await fetch('/api/status');const j=await r.json();
trava.textContent=j.trava;estado.textContent=nomes[j.estado]||j.estado;}catch(e){}}
setInterval(st,1000);st();
</script>
</body></html>)rawliteral";

const char* nomeEstado() {
  switch (estado) {
    case CONTANDO_ATRASO: return "contando";
    case TOCANDO:         return "tocando";
    default:              return "aguardando";
  }
}

void paginaInicial() {
  String html = FPSTR(PAGINA);
  html.replace("%ATRASO_MAX%", String(ATRASO_MAX_MS));
  html.replace("%ATRASO%", String(config.atrasoMs));
  html.replace("%VOLUME%", String(config.volume));
  html.replace("%BRILHO%", String(config.brilho));
  html.replace("%ATIVO%", config.ativo ? "checked" : "");
  server.send(200, "text/html; charset=utf-8", html);
}

void salvar() {
  if (server.hasArg("atraso")) {
    config.atrasoMs = constrain(server.arg("atraso").toInt(), 0, ATRASO_MAX_MS);
  }
  if (server.hasArg("volume")) {
    config.volume = constrain(server.arg("volume").toInt(), 0, 100);
  }
  if (server.hasArg("brilho")) {
    config.brilho = constrain(server.arg("brilho").toInt(), 0, 255);
  }
  // Checkbox desmarcado não é enviado pelo formulário
  config.ativo = server.hasArg("ativo") && server.arg("ativo") != "0";
  salvarConfig();
  Serial.printf("Config salva: atraso=%u ms volume=%u%% brilho=%u ativo=%s\n",
                config.atrasoMs, config.volume, config.brilho, config.ativo ? "sim" : "nao");

  server.sendHeader("Location", "/");
  server.send(303);
}

void apiStatus() {
  char json[160];
  snprintf(json, sizeof(json),
           "{\"trava\":\"%s\",\"estado\":\"%s\",\"atraso\":%u,\"volume\":%u,\"brilho\":%u,\"ativo\":%s}",
           travaFechada ? "fechada" : "aberta", nomeEstado(),
           config.atrasoMs, config.volume, config.brilho, config.ativo ? "true" : "false");
  server.send(200, "application/json", json);
}

void configurarRotas() {
  server.on("/", HTTP_GET, paginaInicial);
  server.on("/salvar", HTTP_POST, salvar);
  server.on("/api/status", HTTP_GET, apiStatus);
  server.on("/testar", HTTP_POST, []() {
    pararCena();
    iniciarContagem();   // simula o destravamento, respeitando o atraso
    server.send(200, "application/json", "{\"ok\":true}");
  });
  server.on("/parar", HTTP_POST, []() {
    pararCena();
    server.send(200, "application/json", "{\"ok\":true}");
  });
  server.onNotFound([]() {
    server.send(404, "text/plain", "Nao encontrado");
  });
}
