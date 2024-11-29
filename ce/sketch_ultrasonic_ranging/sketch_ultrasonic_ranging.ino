#include <Adafruit_NeoPixel.h>

#define PIN 6
#define NUMPIXELS 8
#define DELAYVAL 500

Adafruit_NeoPixel pixels(NUMPIXELS, PIN);

void setup() {
  pixels.begin();
}

void loop() {
  pixels.clear();
  for(int i=0; i<NUMPIXELS; i++) {
    pixels.setPixelColor(i, 0, 0, 150);
    pixels.show();
    delay(DELAYVAL);
  }
}