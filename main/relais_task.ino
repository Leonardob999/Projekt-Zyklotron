#include <Arduino.h>
#include "globals.h"

// ---------- RELAIS PINS ----------
const int relaisPins[4] = { 32, 33, 12, 13 };   // PINS ANPASSEN!
const int relaisAnzahl = 4;

void relaisTask(void *pvParameters) {

  // ---------- INITIALISIERUNG ----------
  for (int i = 0; i < relaisAnzahl; i++) {
    pinMode(relaisPins[i], OUTPUT);
    digitalWrite(relaisPins[i], HIGH);
  }

  int aktuellesRelais = 0;

  // ---------- TASK LOOP ----------
  while (true) {

    int tempo;

    // Tempo threadsicher lesen
    xSemaphoreTake(tempoMutex, portMAX_DELAY);
    tempo = tempoWert;
    xSemaphoreGive(tempoMutex);

    // Tempo 0–100 → Umschaltzeit (ms)
    int intervall = map(tempo, 0, 100, 2000, 100);
    intervall = max(intervall, 50); // Sicherheitsminimum

    // Alle Relais aus
    for (int i = 0; i < relaisAnzahl; i++) {
      digitalWrite(relaisPins[i], HIGH);
    }

    // Aktuelles Relais an
    digitalWrite(relaisPins[aktuellesRelais], LOW);

    // Warten
    vTaskDelay(intervall / portTICK_PERIOD_MS);

    // Nächstes Relais
    aktuellesRelais++;
    if (aktuellesRelais >= relaisAnzahl) {
      aktuellesRelais = 0;
    }
  }
}
