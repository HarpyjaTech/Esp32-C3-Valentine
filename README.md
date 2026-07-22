# 🎁 Caixa Mágica - Dia dos Namorados

<a href="https://buymeacoffee.com/hapyjatech" target="_blank"><img src="https://cdn.buymeacoffee.com/buttons/v2/default-yellow.png" alt="Buy Me A Coffee" style="height: 60px !important;width: 217px !important;" ></a>

## Guia Completo de Montagem

✅ **Caixa:**
- 1x Caixa de presente bonita
- Papel decorativo
- Fita adesiva

Um projeto interativo e romântico baseado no microcontrolador ESP32. Esta "Caixa Mágica" utiliza um sensor de luz (LDR) para detetar o momento exato em que é aberta. Ao ver a luz, a caixa ganha vida: sincroniza a iluminação suave de um LED RGB (em tons pastel) com a reprodução de alta fidelidade (I2S) da melodia *"Enchanted"*, gerando um ambiente perfeito e surpreendente.

## 🌟 Principais Funcionalidades
- **Áudio Hi-Fi via I2S:** Utiliza o módulo DAC PCM5102A e síntese por tabela de ondas (*Wavetable*) para gerar tons senoidais puros, cristalinos e com decaimento suave (efeito aveludado), substituindo os bipes tradicionais.
- **Gatilho Sensível à Luz:** Acionamento automático baseado num limiar analógico ajustável (LDR), mantendo o sistema em espera de forma eficiente.
- **Iluminação Sincronizada:** O LED RGB alterna de forma fluida entre seis cores pastel pré-programadas, mudando a cada duas notas musicais da melodia.
- **Proteção Anti-Bounce:** Lógica de software (`DEBOUNCE_TIME`) implementada para evitar múltiplos acionamentos acidentais num curto espaço de tempo.

## 🛠️ Hardware Necessário
* 1x Placa de Desenvolvimento ESP32
* 1x Módulo DAC I2S PCM5102A
* 1x Sensor LDR (Fotorresistência)
* 1x Resistência de 10kΩ (para o divisor de tensão do LDR)
* 1x LED RGB (Cátodo/Ânodo Comum)
* 3x Resistências de 220Ω a 330Ω (para o LED RGB)
* 1x Altifalante / Amplificador compatível com a saída de linha do PCM5102A

## 🔌 Esquema de Ligações (Pinout)
*Nota: A disposição dos pinos foi estrategicamente escolhida para evitar conflitos com hardware interno (como ecrãs OLED integrados em algumas placas) e pinos de 'strap' críticos durante o boot.*

| Componente | Pino ESP32 | Observação |
| :--- | :--- | :--- |
| **LDR (Sinal)** | `GPIO 32` | Entrada analógica (ADC) perfeitamente segura |
| **LED RGB (Azul)** | `GPIO 25` | Saída PWM |
| **LED RGB (Vermelho)**| `GPIO 26` | Saída PWM |
| **LED RGB (Verde)** | `GPIO 27` | Saída PWM |
| **PCM5102A (DIN)** | `GPIO 18` | Dados de Áudio I2S |
| **PCM5102A (BCK)** | `GPIO 19` | Bit Clock I2S |
| **PCM5102A (LRCK/WS)**| `GPIO 23` | Word Select |

## 💻 Instalação e Configuração do Ambiente
1. Abra o código `caixa_namorados.ino` no **Arduino IDE**.
2. **Requisito Crítico de Software:** Este projeto faz uso da biblioteca de drivers clássica (`<driver/i2s.h>`). Para compilar corretamente e evitar erros de kernel panic no ESP32 (como `CONFLICT! driver_ng is not allowed to be used with the legacy driver`), **certifique-se de fazer o downgrade do pacote "esp32 by Espressif Systems" no Gestor de Placas para a versão `2.0.17`**. As versões `3.x` exigem uma nova sintaxe para o motor I2S e ADC.
3. Selecione a sua placa ESP32 e a respetiva porta COM.
4. Faça a compilação e o carregamento (upload) para o microcontrolador.

## ⚙️ Calibração
Se a caixa for feita de um material mais translúcido ou se a luz ambiente for muito forte, ajuste a sensibilidade do sensor alterando a seguinte variável no código:
```cpp
#define LDR_THRESHOLD 2000 // Aumente o valor (até 4095) para exigir mais luz na abertura; dimi
