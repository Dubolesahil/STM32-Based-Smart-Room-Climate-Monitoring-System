
#include <Wire.h>

#define DHTPIN PB0
#define LCD_ADDR 0x27

#define RS 0x01
#define EN 0x04
#define BL 0x08

uint8_t temperature = 0;
uint8_t humidity = 0;

void expander(uint8_t v) {
  Wire.beginTransmission(LCD_ADDR);
  Wire.write(v | BL);
  Wire.endTransmission();
}

void pulse(uint8_t v) {
  expander(v | EN);
  delayMicroseconds(1);
  expander(v & ~EN);
  delayMicroseconds(50);
}

void nibble(uint8_t v) {
  expander(v);
  pulse(v);
}

void lcdByte(uint8_t v, uint8_t mode) {
  nibble((v & 0xF0) | mode);
  nibble(((v << 4) & 0xF0) | mode);
}

void cmd(uint8_t v) {
  lcdByte(v, 0);
  if (v < 3) delay(2);
}

void printText(const char *s) {
  while (*s) lcdByte(*s++, RS);
}

void number(uint8_t n) {
  if (n >= 100) lcdByte('0' + n / 100, RS);
  if (n >= 10) lcdByte('0' + (n / 10) % 10, RS);
  lcdByte('0' + n % 10, RS);
}

void cursor(uint8_t col, uint8_t row) {
  cmd(0x80 | (row ? 0x40 : 0) | col);
}

void lcdBegin() {
  delay(50);
  pulse(0x30);
  delay(5);
  pulse(0x30);
  delayMicroseconds(150);
  pulse(0x30);
  pulse(0x20);
  cmd(0x28);
  cmd(0x0C);
  cmd(0x06);
  cmd(0x01);
}

bool readDHT() {
  uint8_t d[5] = {0, 0, 0, 0, 0};

  pinMode(DHTPIN, OUTPUT);
  digitalWrite(DHTPIN, LOW);
  delay(20);
  digitalWrite(DHTPIN, HIGH);
  delayMicroseconds(30);
  pinMode(DHTPIN, INPUT_PULLUP);

  uint32_t start = micros();
  while (digitalRead(DHTPIN) == HIGH)
    if (micros() - start > 100) return false;

  start = micros();
  while (digitalRead(DHTPIN) == LOW)
    if (micros() - start > 100) return false;

  start = micros();
  while (digitalRead(DHTPIN) == HIGH)
    if (micros() - start > 100) return false;

  for (uint8_t i = 0; i < 40; i++) {
    start = micros();
    while (digitalRead(DHTPIN) == LOW)
      if (micros() - start > 100) return false;

    start = micros();
    while (digitalRead(DHTPIN) == HIGH)
      if (micros() - start > 100) return false;

    d[i / 8] <<= 1;
    if (micros() - start > 40) d[i / 8] |= 1;
  }

  if ((uint8_t)(d[0] + d[1] + d[2] + d[3]) != d[4])
    return false;

  humidity = d[0];
  temperature = d[2];
  return true;
}

void setup() {
  Wire.begin();
  Wire.setClock(100000);
  lcdBegin();

  cursor(0, 0);
  printText("SMART ROOM");
  cursor(0, 1);
  printText("Starting...");
  delay(1500);
}

void loop() {
  cursor(0, 0);
  printText("                ");
  cursor(0, 1);
  printText("                ");

  if (!readDHT()) {
    cursor(0, 0);
    printText("DHT11 ERROR");
    cursor(0, 1);
    printText("Check sensor");
  } else {
    cursor(0, 0);
    printText("T:");
    number(temperature);
    printText("C H:");
    number(humidity);
    printText("%");

    cursor(0, 1);
    if (temperature >= 30 && humidity > 70)
      printText("WARM & HUMID");
    else if (temperature >= 30)
      printText("Status: WARM");
    else if (humidity > 70)
      printText("Status: HUMID");
    else
      printText("Status: NORMAL");
  }

  delay(2000);
}
