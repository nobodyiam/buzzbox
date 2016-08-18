#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
#include <avr/power.h>
#endif

#define LED_PIN 6
#define NUM_PIXELS 31

#define VOICE_BUSY_PIN 2
#define MANUAL_STOP_PIN 3
#define SOFT_TX 11
#define SOFT_RX 10
#define ALWAYS_HIGH_PIN 9
#define ALWAYS_LOW_PIN 8
#define HIGH_WHEN_ALARM 7

// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_PIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);

// IMPORTANT: To reduce NeoPixel burnout risk, add 1000 uF capacitor across
// pixel power leads, add 300 - 500 Ohm resistor on first pixel's data input
// and minimize distance between Arduino and first pixel.  Avoid connecting
// on a live circuit...if you must, connect GND first.

uint32_t outer_ring_color = strip.Color(234, 0, 71);
uint32_t inner_ring_color = strip.Color(1, 201, 234);
uint32_t none_color = strip.Color(0, 0, 0);

#include <SoftwareSerial.h>
// software serial #1: RX = digital pin 10, TX = digital pin 11
SoftwareSerial portOne(SOFT_RX, SOFT_TX);

boolean commandComplete = false;
int repeat;
int led_mode;
int tts_size;
byte inputbuffer[1];
boolean manualStopped = false;

void setup() {
  // This is for Trinket 5V 16MHz, you can remove these three lines if you are not using a Trinket
#if defined (__AVR_ATtiny85__)
  if (F_CPU == 16000000) clock_prescale_set(clock_div_1);
#endif
  // End of trinket special code

  pinMode(VOICE_BUSY_PIN, INPUT);
  pinMode(MANUAL_STOP_PIN, OUTPUT);
  pinMode(ALWAYS_HIGH_PIN, OUTPUT);
  pinMode(HIGH_WHEN_ALARM, OUTPUT);
  pinMode(ALWAYS_LOW_PIN, OUTPUT);

  digitalWrite(MANUAL_STOP_PIN, LOW);
  digitalWrite(ALWAYS_HIGH_PIN, HIGH);
  digitalWrite(ALWAYS_LOW_PIN, LOW);
  digitalWrite(HIGH_WHEN_ALARM, LOW);

  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
  Serial.begin(9600);

  // Start each software serial port
  portOne.begin(9600);

  attachInterrupt(digitalPinToInterrupt(MANUAL_STOP_PIN), manualStop, RISING);

  reset();
  Serial.println("Set up done!");
}
byte voice_stop[5] = {0xFD, 0x00, 0x02, 0x03, 0xFC};

void loop() {
  /*Serial.println("Reading input");
    Serial.readBytesUntil(END_SYMBOL, input_tts, TTS_SIZE);
    for(count = 0; count < sizeof(input_tts); count++) {
    Serial.print(input_tts[count], HEX);
    }
    Serial.println("\n");*/
  while (Serial.available() && !commandComplete) {
    // get the new byte:
    char inChar = (char)Serial.read();

    //#{repeat}{led-mode}{tts-length}{tts-content}
    if (inChar == '#') { //command start
      Serial.print("Repeat: ");
      Serial.readBytes(inputbuffer, 1);
      repeat = inputbuffer[0];
      Serial.println(repeat, DEC);

      Serial.print("Led mode: ");
      Serial.readBytes(inputbuffer, 1);
      led_mode = inputbuffer[0];
      Serial.println(led_mode, DEC);

      Serial.print("TTS Size: ");
      Serial.readBytes(inputbuffer, 1);
      tts_size = inputbuffer[0];
      Serial.println(tts_size, DEC);

      int tts_wait_time = 0;
      while (Serial.available() < tts_size && tts_wait_time < 1000) {
        tts_wait_time += 50;
        delay(50);
      }

      if (Serial.available() >= tts_size) {
        commandComplete = true;
      } else {
        reset();
      }
    } else {
      Serial.println("Unknown: " + String(inChar));
    }

  }

  if (commandComplete) {
    byte tts_content[tts_size];

    if (Serial.readBytes(tts_content, tts_size) != tts_size) {
      reset();
      return;
    }

    digitalWrite(HIGH_WHEN_ALARM, HIGH);

    int voice_busy = 0;
    int voice_started = 0;
    int voice_wait_time = 0;
    int led_counter = 0;
    for (int counter = 0; counter < repeat; counter++) {
      // manually stopped or something is wrong
      if (manualStopped == true || led_counter > 20) {
        break;
      }
      if (voice_started == 0) {
        handleTTS(tts_content);
     
        voice_busy = digitalRead(VOICE_BUSY_PIN);
        while (voice_busy == 0 && voice_wait_time < 1000) {
          voice_wait_time += 50;
          delay(50);
          voice_busy = digitalRead(VOICE_BUSY_PIN);
        }

        if (voice_busy == 1) {
          voice_started = 1;
        }

        handleLedMode(led_mode);
        led_counter++;
      } else {
        voice_busy = digitalRead(VOICE_BUSY_PIN);

        if (voice_busy == 1) {
          counter--;
          handleLedMode(led_mode);
          led_counter++;
        } else {
          voice_started = 0;
        }
      }
    }

    reset();
    digitalWrite(HIGH_WHEN_ALARM, LOW);
  }
}

void reset() {
  portOne.write(voice_stop, sizeof(voice_stop));
  colorWipe(strip.Color(0, 0, 0), 0);
  repeat = 0;
  led_mode = 0;
  tts_size = 0;
  commandComplete = false;
  manualStopped = false;
}

int handleLedMode(int mode) {
  switch (mode) {
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

void handleTTS(byte tts[]) {
  portOne.write(tts, sizeof(tts));
}

void manualStop() {
  if (digitalRead(MANUAL_STOP_PIN) == HIGH) {
    Serial.println("manully stopped!");
    manualStopped = true;
  }
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
  for (int q = 0; q < 2; q++) {
    for (uint16_t i = 0; i < 24; i = i + 2) {
      strip.setPixelColor(i + q, outer_ring_color);
    }
    if (q % 2 == 0) {
      wipe_inner_ring_color(inner_ring_color);
    } else {
      wipe_inner_ring_color(none_color);
    }
    strip.show();

    delay(300);
    for (uint16_t i = 0; i < 24; i = i + 1) {
      strip.setPixelColor(i + q, 0);
    }
    strip.show();
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
