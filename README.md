# 🎁 Caixa Mágica - Dia dos Namorados

<a href="https://buymeacoffee.com/hapyjatech" target="_blank"><img src="https://cdn.buymeacoffee.com/buttons/v2/default-yellow.png" alt="Buy Me A Coffee" style="height: 60px !important;width: 217px !important;" ></a>

## 📦 Projetos neste repositório

| Projeto | Placa | Descrição |
|---------|-------|-----------|
| [Caixa Mágica - Dia dos Namorados](#guia-completo-de-montagem) (`caixa_namorados.ino`) | ESP32-C3 | Sensor LDR detecta a abertura, buzzer toca "Enchanted" e LED RGB alterna cores |
| [Caixa de Presente "Swim"](caixa_swim_esp32s3/README.md) (`caixa_swim_esp32s3/`) | ESP32-S3 | Trava de metal detecta o destravamento, buzzer toca "Swim" - BTS e fita LED vai do vermelho ao violeta conforme o volume; atraso ajustável via web server |

---

## Guia Completo de Montagem

---

## 📋 Componentes Necessários

✅ **Eletrônicos:**
- 1x ESP32-C3 (microcontrolador)
- 1x Buzzer Passivo (5V)
- 1x LED RGB comum ânodo ou cátodo comum
- 3x Resistores 220Ω (para o LED RGB)
- 1x Resistor 10kΩ (para LDR)
- 1x Sensor LDR (fotoresistor)
- Fios jumper
- Bateria ou cabo USB (alimentação)

✅ **Caixa:**
- 1x Caixa de presente bonita
- Papel decorativo
- Fita adesiva

---

## 🔌 Esquema de Conexão

### **ESP32-C3 Pinout:**
```
ESP32-C3
┌─────────────────────┐
│                     │
│  [USB]    GND  5V   │
│                     │
│ GPIO5 (LED_B)      │
│ GPIO6 (LED_R)      │
│ GPIO7 (LED_G)      │
│ GPIO8 (BUZZER)     │
│ GPIO4 (LDR)        │
│                     │
└─────────────────────┘
```

### **Conexões Detalhadas:**

#### 1️⃣ **LED RGB** (assumindo cátodo comum - pino mais longo)
```
LED RGB (Cátodo Comum)
    │
    ├─ R (vermelho)    → Resistor 220Ω → GPIO6
    ├─ G (verde)       → Resistor 220Ω → GPIO7
    ├─ B (azul)        → Resistor 220Ω → GPIO5
    └─ Cátodo (K)      → GND
```

#### 2️⃣ **Buzzer Passivo**
```
Buzzer
  ├─ Positivo (+) → GPIO8
  └─ Negativo (-) → GND
```

#### 3️⃣ **Sensor LDR**
```
LDR + Resistor 10kΩ (Divisor de Tensão)
  
  5V ─── LDR ─┬─ GPIO4 (entrada analógica)
              │
          10kΩ Resistor
              │
             GND
```

---

## 💡 Funcionamento do Sensor LDR

O sensor LDR é uma **fotoresistência** que muda sua resistência conforme a luz:
- **Luz fraca** = alta resistência → leitura analógica baixa (0-2000)
- **Luz forte** = baixa resistência → leitura analógica alta (2000-4095)

Quando você **abre a caixa**, a luz entra e ativa a sequência.

**Ajuste recomendado:**
- Teste o código com a caixa fechada e anote o valor (ex: 800)
- Teste com a caixa aberta e anote o valor (ex: 3000)
- Use o valor intermediário como `LDR_THRESHOLD`

---

## 🛠️ Passo a Passo de Montagem

### **Montagem Eletrônica:**

1. **Preparar o ESP32-C3:**
   - Solde os pinos headers se necessário
   - Conecte em uma protoboard para facilitar

2. **LED RGB:**
   - Identifique o cátodo comum (pino mais longo)
   - Conecte cada cor com seu respectivo resistor
   - A ordem das cores pode ser RGB ou BGR (ajuste no código se necessário)

3. **Buzzer:**
   - Conecte positivo a GPIO8
   - Conecte negativo a GND
   - Teste com um `digitalWrite(8, HIGH)` primeiro

4. **Sensor LDR:**
   - Monte o divisor de tensão conforme esquema
   - Posicione o LDR virado para cima (para receber luz)

5. **Bateria/Alimentação:**
   - Use uma bateria 5V/1A ou USB
   - Conecte em 5V e GND do ESP32

### **Montagem na Caixa:**

1. **Fazer orifícios:**
   - Pequeno furo para o LED RGB (colado na interna)
   - Pequeno furo para o LDR (detector de abertura)
   - Deixar fios dentro da caixa

2. **Posicionar componentes:**
   - LED RGB: cole na parte interna superior da caixa
   - LDR: posicione para cima/lateral (receberá luz ao abrir)
   - Protoboard: fixe com velcro reutilizável no fundo

3. **Decor:**
   - Coloque papel decorativo cobrindo os componentes
   - Adicione luzes decorativas se desejar
   - Deixe lindo! 💕

---

## 📲 Carregando o Código

### **Opção 1: Arduino IDE**
```
1. Instale: ESP32 by Espressif Systems
2. Placa: ESP32-C3 Dev Module
3. Porta: Selecione a correta
4. Copie o código em caixa_namorados.ino
5. Compile e Carregue
```

### **Opção 2: PlatformIO**
```
[env:esp32-c3-devkitm-1]
platform = espressif32
board = esp32-c3-devkitm-1
framework = arduino
```

---

## 🎵 Personalizando a Melodia

Se quiser mudar a melodia "Enchanted" ou adicionar outra:

1. **Encontre as frequências da nota** (em Hz):
   - C4: 262, D4: 294, E4: 330, F4: 349, G4: 392, A4: 440, B4: 494
   - C5: 523, D5: 587, E5: 659, F5: 698, G5: 784, A5: 880, B5: 988
   - C6: 1047, ...

2. **Atualize os arrays no código:**
```cpp
const int melody[] = {523, 587, 659, ...};
const int noteDurations[] = {400, 400, 400, ...};
```

3. **Durações em ms:**
   - 200: nota rápida
   - 400: nota média
   - 800: nota longa

---

## 🔧 Troubleshooting

| Problema | Solução |
|----------|---------|
| LED não acende | Verifique polaridade, teste cada cor individualmente |
| Buzzer não faz som | Verifique se é passivo (não ativo), teste frequência |
| LDR não detecta | Ajuste `LDR_THRESHOLD` no código, teste valores |
| ESP32 não liga | Verifique alimentação 5V, tente porta USB diferente |
| Melodia incorreta | Ajuste as frequências e durações das notas |

---

## 💝 Dicas Finais

✨ **Para mais efeito:**
- Use cores RGB suaves (pastéis)
- Posicione o LED para iluminar a caixa
- Adicione pétalas de rosa ou papel decorativo
- Deixe um cartão bonito dentro da caixa

🎁 **Teste tudo antes de presentear:**
- Feche a caixa no escuro
- Abra lentamente e veja a mágica acontecer
- Ajuste o tempo e cores conforme necessário

Bom presente! 🌹💕
