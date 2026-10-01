# 🎁 Caixa de Presente "Swim" - ESP32-S3

Caixa de madeira com trava de metal. Ao **destravar** a caixa, depois de um atraso de **500 ms** (ajustável pelo celular), um **buzzer passivo** toca **"Swim" - BTS** e uma **fita LED RGB** colada nas laterais internas da caixa muda de cor acompanhando o **volume** do som:

```
volume baixo  ───────────────────────────────▶  volume alto
 VERMELHO → laranja → amarelo → verde → azul → VIOLETA
```

Tudo é controlado por um **ESP32-S3**, que também cria um **web server** para alterar o atraso, o volume, o brilho e testar a cena.

---

## 📋 Componentes

✅ **Eletrônicos:**
- 1x ESP32-S3 (ex.: ESP32-S3-DevKitC-1)
- 1x Buzzer passivo
- 1x Fita LED RGB comum, **não endereçável** (ex.: 5050), com 4 pinos: **+V, R, G, B**
- 3x MOSFET canal N logic-level (IRLZ44N, IRLB8721, AO3400…)
- 3x Resistor 220Ω (gate dos MOSFETs)
- 3x Resistor 10kΩ (pull-down dos gates)
- Fios, solda e terminais/olhal para a trava
- Fonte com a tensão da fita (12V ou 5V); se for 12V, um regulador step-down 12V→5V para o ESP32

✅ **Caixa:**
- 1x Caixa de madeira com tampa
- 1x Trava de metal (fecho tipo ferrolho, "hasp" ou fecho de pressão)
- Fita dupla-face / cola quente para a fita LED

---

## 🔌 Esquema de Ligação

### **ESP32-S3 Pinout usado:**
```
ESP32-S3
┌──────────────────────────┐
│ [USB]        GND   5V    │
│                          │
│ GPIO5  ← TRAVA (contato) │
│ GPIO4  → BUZZER          │
│ GPIO15 → MOSFET R        │
│ GPIO16 → MOSFET G        │
│ GPIO17 → MOSFET B        │
└──────────────────────────┘
```

#### 1️⃣ **Trava de metal (detecção de abertura)**
A própria trava funciona como um interruptor: as duas partes metálicas só se tocam quando a caixa está **travada**.

```
Parte fixa da trava (na caixa)  ── fio ──▶ GND
Parte móvel da trava (na tampa) ── fio ──▶ GPIO5   (pull-up interno ativado)

Travada    → metal encosta → GPIO5 = LOW  → aguardando
Destravada → metal separa  → GPIO5 = HIGH → atraso de 500 ms → cena!
```

> 💡 Prenda os fios com parafuso + terminal olhal ou solda. Se a trava for pintada/oxidada, lixe o ponto de contato.
> Se o contato for ruim, use uma **chave reed + ímã** ou um **microswitch** acionado pela trava — a ligação é a mesma (GPIO5 e GND).

#### 2️⃣ **Buzzer passivo**
```
Buzzer
  ├─ Positivo (+) → GPIO4
  └─ Negativo (-) → GND
```
> Para mais volume, acione o buzzer por um transistor NPN (BC547/2N2222) alimentado em 5V.

#### 3️⃣ **Fita LED RGB (pinos +V, R, G, B)**
O ESP32 não aguenta a corrente da fita direto nos pinos, então cada cor passa por um MOSFET, que liga o pino R, G ou B ao GND com PWM:

```
Fonte +12V (ou +5V) ──────────── +V da fita

Fita R ── Dreno MOSFET 1 │ Fonte → GND │ Gate ── 220Ω ── GPIO15  (10kΩ gate→GND)
Fita G ── Dreno MOSFET 2 │ Fonte → GND │ Gate ── 220Ω ── GPIO16  (10kΩ gate→GND)
Fita B ── Dreno MOSFET 3 │ Fonte → GND │ Gate ── 220Ω ── GPIO17  (10kΩ gate→GND)

GND da fonte ── GND do ESP32 (GND comum obrigatório)
```

> Com fonte de 12V, alimente o ESP32 por um step-down 12V→5V no pino 5V (ou pelo USB).
> O pull-down de 10kΩ mantém a fita apagada enquanto o ESP32 inicia.

---

## 🎵 Como a cor acompanha o volume

O buzzer passivo é acionado por PWM: a **frequência** define a nota e o **duty cycle** define o volume (0% = mudo, 50% = máximo).

Cada nota da melodia tem um volume próprio (0–100) e um envelope (ataque → leve decaimento → soltura). A cada 10 ms o ESP32 calcula o volume que está sendo tocado e converte em cor:

| Volume | Cor |
|--------|-----|
| 0% (pausa / muito baixo) | 🔴 Vermelho |
| ~25% | 🟡 Amarelo |
| ~50% | 🟢 Verde / ciano |
| ~75% | 🔵 Azul |
| 100% (mais alto) | 🟣 Violeta |

A cor usa o volume **da música** (antes do volume máximo configurado), então mesmo com o buzzer mais baixo a fita continua percorrendo do vermelho ao violeta.

---

## 📲 Web Server

Ao ligar, a caixa cria a rede Wi-Fi:

- **Rede:** `Caixa-Swim`
- **Senha:** `swim2026`
- **Endereço:** http://192.168.4.1 (ou http://caixa.local)

Para usar o Wi-Fi de casa, preencha `WIFI_SSID` e `WIFI_SENHA` no código (o IP aparece no Serial Monitor). Se não conectar em 10 s, a caixa volta a criar a própria rede.

Na página é possível:
- ⏱️ Alterar o **atraso** após destravar (0 a 10000 ms, padrão **500 ms**)
- 🔊 Ajustar o **volume máximo** do buzzer
- 💡 Ajustar o **brilho** da fita
- ✅ Ligar/desligar a cena ao destravar
- ▶️ **Testar** a cena (simula o destravamento, com o atraso) e ■ **Parar**
- 👀 Ver o estado da trava em tempo real

As configurações ficam salvas na memória (NVS) e continuam valendo depois de desligar.

**API (para automações):**
| Rota | Método | Descrição |
|------|--------|-----------|
| `/api/status` | GET | JSON com trava, estado e configurações |
| `/salvar` | POST | `atraso`, `volume`, `brilho`, `ativo=1` |
| `/testar` | POST | Inicia a cena (com atraso) |
| `/parar` | POST | Para a cena |

Exemplo: `curl -X POST -d "atraso=800&volume=100&brilho=180&ativo=1" http://caixa.local/salvar`

---

## 🎬 Comportamento

1. Caixa **travada** → tudo apagado, aguardando.
2. Caixa **destravada** → espera o atraso configurado (500 ms).
3. Buzzer toca "Swim" e a fita muda de vermelho a violeta conforme o volume.
4. No fim da música tudo apaga. Para tocar de novo, trave e destrave a caixa.
5. Se a caixa for **travada novamente** durante a música (ou durante o atraso), a cena para na hora.

---

## 📥 Carregando o Código

### **Arduino IDE**
```
1. Instale a placa: "esp32 by Espressif Systems" (versão 3.x)
2. Placa: ESP32S3 Dev Module
3. Abra caixa_swim_esp32s3/caixa_swim_esp32s3.ino
4. Compile e carregue (nenhuma biblioteca extra é necessária)
```

### **PlatformIO**
```ini
[env:esp32-s3-devkitc-1]
platform = espressif32
board = esp32-s3-devkitc-1
framework = arduino
monitor_speed = 115200
```
> O código usa a API LEDC do Arduino-ESP32 **3.x** (`ledcAttach`, `ledcChangeFrequency`). Use uma versão de `platform` baseada no core 3.x.

---

## 🎼 Ajustando a Melodia

A melodia fica no array `melodia[]`. Cada nota é `{frequência, duração, volume}`:

```cpp
{NOTE_C6, 4, 100},   // Dó6, 4 semicolcheias, volume máximo (violeta)
{NOTE_E5, 2, 35},    // Mi5, 2 semicolcheias, volume baixo (vermelho/laranja)
{REST,    2, 0},     // pausa
```

- **Andamento:** `#define BPM 96`
- **Duração:** em semicolcheias (2 = colcheia, 4 = semínima, 8 = mínima)
- **Volume (0–100):** controla ao mesmo tempo a intensidade do buzzer e a cor da fita

O arranjo incluído é uma versão simplificada para buzzer (uma nota por vez). Ouça a música e ajuste notas e durações até ficar do seu jeito.

---

## 🔧 Troubleshooting

| Problema | Solução |
|----------|---------|
| Cena não inicia ao destravar | Veja "Trava" na página web; se aparecer invertido, troque `TRAVA_FECHADA_NIVEL` para `HIGH` |
| Cena dispara sozinha | Melhore o contato da trava ou aumente `DEBOUNCE_MS` |
| Fita não acende | Confira o GND comum entre fonte, MOSFETs e ESP32, e se o MOSFET é logic-level |
| Cores trocadas (ex.: vermelho aparece verde) | Troque os fios R/G/B da fita ou os números em `PIN_FITA_R/G/B` |
| Fita fica acesa direto | MOSFET ligado errado (dreno/fonte invertidos) ou sem pull-down no gate |
| ESP32 reinicia ao acender a fita | Fonte fraca — use uma fonte com mais corrente ou reduza o brilho |
| Buzzer baixo | Aumente o volume na página ou use um transistor em 5V |
| Não encontro a página | Conecte na rede `Caixa-Swim` e abra http://192.168.4.1 |

Bom presente! 💜🎶
