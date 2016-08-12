#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
#include <avr/power.h>
#endif

#define PIN 6
#define VOICE_BUSY_PIN 2

// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(31, PIN, NEO_GRB + NEO_KHZ800);

// IMPORTANT: To reduce NeoPixel burnout risk, add 1000 uF capacitor across
// pixel power leads, add 300 - 500 Ohm resistor on first pixel's data input
// and minimize distance between Arduino and first pixel.  Avoid connecting
// on a live circuit...if you must, connect GND first.

const int LED_START = 94; //^
const int TTS_START = 126; //~
const int END_SYMBOL = 36; //$
const int TTS_SIZE = 50;

uint32_t outer_ring_color = strip.Color(234, 0, 71);
uint32_t inner_ring_color = strip.Color(1, 201, 234);
uint32_t none_color = strip.Color(0, 0, 0);

#include <SoftwareSerial.h>
// software serial #1: RX = digital pin 10, TX = digital pin 11
SoftwareSerial portOne(10, 11);

int input;
int led_mode;
int input_led_mode;
byte tts[TTS_SIZE];
byte input_tts[TTS_SIZE];
int count;
int tts_set = 0;
int voice_busy = 0;     // variable to store the read value
int repeat = 3;

void setup() {
  // This is for Trinket 5V 16MHz, you can remove these three lines if you are not using a Trinket
#if defined (__AVR_ATtiny85__)
  if (F_CPU == 16000000) clock_prescale_set(clock_div_1);
#endif
  // End of trinket special code

 pinMode(VOICE_BUSY_PIN, INPUT);
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
  Serial.begin(9600);
  // Start each software serial port
  portOne.begin(9600);
  Serial.println("Set up done!");
}
byte text[10] = {0xFD,0x00,0x07,0x01,0x01,0xC4,0xE3,0xBA,0xC3,0xA4};
void loop() {
  //Serial.println("loop");
  voice_busy = digitalRead(VOICE_BUSY_PIN);
  //Serial.print("Voice busy: ");
  //Serial.println(voice_busy);

  /*if (repeat <= 0) {
    stop();
  }

  if (tts_set == 1 && !voice_busy) {
    repeat--;
  }

  if (tts_set == 0 && led_mode != 0) {
    repeat--;
  }*/
  
  input_led_mode = led_mode;
  if (Serial.available()) {
    input = Serial.read();
    switch (input) {
      case LED_START:
        Serial.println("Led Start");
        input_led_mode = Serial.parseInt();
        break;
      case TTS_START:
        Serial.println("TTS Start");
        if (Serial.readBytesUntil(END_SYMBOL, input_tts, TTS_SIZE) > 0) {
          tts_set = 1;
          arrayCopy(input_tts, tts, TTS_SIZE);
        }
        break;
      case END_SYMBOL:
        input_led_mode = 0;
        Serial.println("Stop");
        break;
      default:
        Serial.println("Unknown: " + String(char(input)));
        break;
    }
  }

  led_mode = handleLedMode(input_led_mode);
  Serial.print("led mode: ");
  Serial.println(led_mode);
  /*if(voice_busy == 0) {
    handleTTS();
  }*/
}

int handleLedMode(int mode) {
  Serial.print("input mode: ");
  Serial.println(mode);
  switch (mode) {
    case 0:
      stop();
      break;
    case 1:
      alarm();
      break;
    case 2:
      blingbling();
      break;
    case 3:
      marquee();
      break;
    default:
      return led_mode;
  }
  return mode;
}

void handleTTS() {
  if (tts_set == 0) {
    return;
  }
  Serial.println("Handle TTS");
  portOne.write(text, sizeof(text));
  Serial.println("Handle TTS Done");
}

void arrayCopy(byte src[], byte target[], int limit) {
  for (count = 0; count < limit; count++) {
    target[count] = src[count];
  }
}

void stop() {
  colorWipe(strip.Color(0, 0, 0), 0);
  tts_set = 0;
  led_mode = 0;
}

void wipe_outer_ring_color(uint32_t c) {
  for (uint16_t i = 0; i < 24; i++) {
    strip.setPixelColor(i, c);
  }
}

void wipe_inner_ring_color(uint32_t c) {
  for (uint16_t i = 24; i < 31; i++) {
    strip.setPixelColor(i, c);
  }
}

void alarm() {
  wipe_inner_ring_color(none_color);
  wipe_outer_ring_color(outer_ring_color);
  strip.show();
  delay(300);
  wipe_inner_ring_color(inner_ring_color);
  wipe_outer_ring_color(none_color);
  strip.show();
  delay(300);
}

void blingbling() {
  wipe_inner_ring_color(inner_ring_color);
  for (int q = 0; q < 2; q++) {
    for (uint16_t i = 0; i < 24; i = i + 2) {
      strip.setPixelColor(i + q, outer_ring_color);
    }
    strip.show();

    delay(300);

    for (uint16_t i = 0; i < 24; i = i + 1) {
      strip.setPixelColor(i + q, 0);
    }
  }
}

void marquee() {
  for (uint16_t i = 0; i < 24; i++) {
    strip.setPixelColor(i, outer_ring_color);
    strip.setPixelColor(i % 6 + 25, inner_ring_color);
    int j = i - 1;
    if (j < 0) {
      j += 24;
    }
    strip.setPixelColor(j, none_color);
    strip.setPixelColor(j % 6 + 25, none_color);
    strip.setPixelColor(24, inner_ring_color);
    strip.show();
    delay(50);
  }
}

// Fill the dots one after the other with a color
void colorWipe(uint32_t c, uint8_t wait) {
  for (uint16_t i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, c);
    strip.show();
    delay(wait);
  }
}
