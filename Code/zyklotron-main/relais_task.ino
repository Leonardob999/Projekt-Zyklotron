#include <Arduino.h>
#include "globals.h"

// ---------- RELAIS PINS ----------
const int relaisPins[4] = { 32, 33, 15, 2 };
const int hallPins[4] = { 4, 16, 5, 17 };
const int relaisAnzahl = 4;

volatile bool relaisAktiv[4] = { false, false, false, false };
volatile uint32_t relaisAusZeit[4];



void IRAM_ATTR relaisTrigger(uint8_t index) {
  uint32_t jetzt = millis();

  int tempoLok;
  if (xSemaphoreTakeFromISR(tempoMutex, NULL) == pdTRUE) {
    tempoLok = tempoWert;
    xSemaphoreGiveFromISR(tempoMutex, NULL);
  } else {
    tempoLok = 0;
  }

  // 0–100 % → 0–30 ms
  int relaisZeitMs = map(tempoLok, 0, 100, 0, 100);
  relaisZeitMs = constrain(relaisZeitMs, 0, 100);

  digitalWrite(relaisPins[index], LOW); // Relais EIN
  relaisAktiv[index] = true;
  relaisAusZeit[index] = jetzt + relaisZeitMs;
}

void IRAM_ATTR hallISR0() {
  relaisTrigger(0);
}
void IRAM_ATTR hallISR1() {
  relaisTrigger(1);
}
void IRAM_ATTR hallISR2() {
  relaisTrigger(2);
}
void IRAM_ATTR hallISR3() {
  relaisTrigger(3);
}

void relaisTask(void *pvParameters) {

  // ---------- INITIALISIERUNG ----------
  for (int i = 0; i < relaisAnzahl; i++) {
    pinMode(relaisPins[i], OUTPUT);
    digitalWrite(relaisPins[i], HIGH);  // Relais AUS
  }

  for (int i = 0; i < 4; i++) {
    pinMode(hallPins[i], INPUT_PULLDOWN);
  }

  // ---------- INTERRUPTS ----------
  attachInterrupt(digitalPinToInterrupt(hallPins[0]), hallISR0, FALLING);
  attachInterrupt(digitalPinToInterrupt(hallPins[1]), hallISR1, FALLING);
  attachInterrupt(digitalPinToInterrupt(hallPins[2]), hallISR2, FALLING);
  attachInterrupt(digitalPinToInterrupt(hallPins[3]), hallISR3, FALLING);

  // ---------- TASK LOOP ----------
  while (true) {
    uint32_t jetzt = millis();

    for (int i = 0; i < relaisAnzahl; i++) {
      if (relaisAktiv[i] && jetzt >= relaisAusZeit[i]) {
        digitalWrite(relaisPins[i], HIGH);  // Relais AUS
        relaisAktiv[i] = false;
      }
    }

    vTaskDelay(1 / portTICK_PERIOD_MS);
  }
}
