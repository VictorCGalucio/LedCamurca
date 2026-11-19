#include <IRremote.hpp>
#include <Adafruit_NeoPixel.h>

#define IR_PIN   2
#define DATA_PIN 6
#define N_PIXELS 30   // ajuste para a sua fita
#define REFRESH_MS 0          // 0 = nunca reenvia config; ex.: 5000 = a cada 5 s

// NEO_RGB: o TM1903 espera R-G-B. NEO_KHZ800: SET flutuando (modo rápido).
Adafruit_NeoPixel strip(N_PIXELS, DATA_PIN, NEO_RGB + NEO_KHZ800);

uint8_t cor[3] = {255, 255, 255};
uint8_t brilho = 128;   // 0..255
bool ligado = true;

uint32_t ultimoRefresh = 0;
uint8_t  ultimoCmd = 0;

void enviaConfig() {
  for (int i = 0; i < N_PIXELS; i++) strip.setPixelColor(i, 0xFF, 0x52, 0x94);
  strip.show();
  delayMicroseconds(100);
}

// Só desenha o estado atual. Não altera cor, brilho nem ligado.
void render() {
  uint8_t r = 0, g = 0, b = 0;
  if (ligado) {
    r = (uint16_t)cor[0] * brilho / 255;
    g = (uint16_t)cor[1] * brilho / 255;
    b = (uint16_t)cor[2] * brilho / 255;
  }
  if (r > 254) r = 254;
  for (int i = 0; i < N_PIXELS; i++) strip.setPixelColor(i, r, g, b);
  strip.show();
}

void refreshCompleto() { enviaConfig(); render(); }

uint32_t tRefresh = 0;

void setup() {
  Serial.begin(115200);
  strip.begin();
  delay(200);                                  // tempo para os TM1903 iniciarem
  IrReceiver.begin(IR_PIN, ENABLE_LED_FEEDBACK);
  refreshCompleto(); 
}

void loop() {
  if (IrReceiver.decode()) {
    bool repeat = IrReceiver.decodedIRData.flags & IRDATA_FLAGS_IS_REPEAT;
    uint8_t cmd = repeat ? ultimoCmd : IrReceiver.decodedIRData.command;
    IrReceiver.resume();

    bool mudou = true;
    if (repeat && cmd != 0x09 && cmd != 0x1D) {
      mudou = false;
    } else {
      ultimoCmd = cmd;
      Serial.print("Comando: 0x"); Serial.println(cmd, HEX);
      switch (cmd) {
        case 0x1F: ligado = false; break;
        case 0x0D: ligado = true;  break;
        case 0x09: brilho = min(brilho + 25, 255); break;
        case 0x1D: brilho = max(brilho - 25, 10);  break; 
        case 0x11: cor[0]=255; cor[1]=0;   cor[2]=0;   break;  // azul
        case 0x19: cor[0]=0;   cor[1]=255; cor[2]=0;   break;  // vermelho
        case 0x1B: cor[0]=0;   cor[1]=0;   cor[2]=255; break;  // verde
        case 0xE: cor[0]=180; cor[1]=255; cor[2]=0; ligado = true; break;  // roxo
        case 0x15: cor[0]=255; cor[1]=255; cor[2]=255; ligado = true; break; 
        default:   mudou = false;
      }
    }
    if (mudou) { render(); tRefresh = millis(); }
  }

  // keep-alive: reenvia o estado atual sem alterá-lo
  if (IrReceiver.isIdle() && millis() - tRefresh > 200) {
    render();
    tRefresh = millis();
  }
}
