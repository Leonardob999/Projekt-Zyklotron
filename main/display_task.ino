#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP32Encoder.h>

#include "globals.h"

// ---------- DISPLAY ----------
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// ---------- PINBELEGUNG ----------
const int CLK = 25;
const int DT  = 26;
const int SW  = 14;

// ---------- ENCODER ----------
ESP32Encoder encoder;

// ---------- EINSTELLUNGEN ----------
const int minWert = 0;
const int maxWert = 100;

// ---------- LAYOUT-TYPEN ----------
enum SeitenLayout {
  LAYOUT_BALKEN,
  LAYOUT_GROSSZAHL,
  LAYOUT_TEXT
};

// ---------- SEITENSTRUKTUR ----------
struct Seite {
  const char* titel;
  SeitenLayout layout;
  int wert;
};

// ---------- SEITEN DEFINIEREN ----------
Seite seiten[] = {
  { "Lautstaerke", LAYOUT_BALKEN, 50 },
  { "Helligkeit", LAYOUT_BALKEN, 75 },
  { "Tempo",      LAYOUT_GROSSZAHL, 90 },
  { "Status",     LAYOUT_TEXT, 0 }
};

const int anzahlSeiten = sizeof(seiten) / sizeof(seiten[0]);

// ---------- VARIABLEN ----------
int aktuelleSeite = 0;
long letzteEncoderPos = 0;

bool editModus = false;
bool letzterButtonStatus = HIGH;
unsigned long letzteButtonZeit = 0;
const unsigned long entprellZeit = 200;

// ---------- FUNKTIONSDEKLARATIONEN ----------
void zeichneAktuelleSeite();
void zeichneBalkenSeite(Seite &s);
void zeichneGrosszahlSeite(Seite &s);
void zeichneTextSeite(Seite &s);

// ======================================================
// ================== DISPLAY TASK ======================
// ======================================================

void displayTask(void *pvParameters) {

  // ---------- INITIALISIERUNG ----------
  Wire.begin();
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();

  pinMode(SW, INPUT_PULLUP);

  ESP32Encoder::useInternalWeakPullResistors = puType::up;
  encoder.attachHalfQuad(DT, CLK);
  encoder.clearCount();

  zeichneAktuelleSeite();

  // ---------- TASK LOOP ----------
  while (true) {

    // ----- BUTTON -----
    bool button = digitalRead(SW);
    if (button == LOW && letzterButtonStatus == HIGH) {
      if (millis() - letzteButtonZeit > entprellZeit) {
        editModus = !editModus;
        letzteButtonZeit = millis();
        zeichneAktuelleSeite();
      }
    }
    letzterButtonStatus = button;

    // ----- ENCODER -----
    long pos = encoder.getCount() / 2;
    if (pos != letzteEncoderPos) {
      int richtung = (pos > letzteEncoderPos) ? 1 : -1;

      if (!editModus) {
        aktuelleSeite += richtung;
        if (aktuelleSeite >= anzahlSeiten) aktuelleSeite = 0;
        if (aktuelleSeite < 0) aktuelleSeite = anzahlSeiten - 1;
      } else {
        seiten[aktuelleSeite].wert += richtung;
        if (seiten[aktuelleSeite].wert > maxWert) seiten[aktuelleSeite].wert = maxWert;
        if (seiten[aktuelleSeite].wert < minWert) seiten[aktuelleSeite].wert = minWert;

        // ----- TEMPO THREADSICHER ÜBERNEHMEN -----
        if (strcmp(seiten[aktuelleSeite].titel, "Tempo") == 0) {
          xSemaphoreTake(tempoMutex, portMAX_DELAY);
          tempoWert = seiten[aktuelleSeite].wert;
          xSemaphoreGive(tempoMutex);
        }
      }

      letzteEncoderPos = pos;
      zeichneAktuelleSeite();
    }

    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

// ======================================================
// ================== ZEICHENFUNKTIONEN =================
// ======================================================

void zeichneAktuelleSeite() {
  display.clearDisplay();

  // Überschrift
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(seiten[aktuelleSeite].titel);
  display.drawLine(0, 10, 127, 10, SSD1306_WHITE);

  switch (seiten[aktuelleSeite].layout) {
    case LAYOUT_BALKEN:
      zeichneBalkenSeite(seiten[aktuelleSeite]);
      break;

    case LAYOUT_GROSSZAHL:
      zeichneGrosszahlSeite(seiten[aktuelleSeite]);
      break;

    case LAYOUT_TEXT:
      zeichneTextSeite(seiten[aktuelleSeite]);
      break;
  }

  display.display();
}

void zeichneBalkenSeite(Seite &s) {
  display.setCursor(0, 20);
  display.print("Wert: ");
  display.print(s.wert);
  display.print("%");

  display.drawRect(0, 35, 128, 10, SSD1306_WHITE);
  int breite = map(s.wert, 0, 100, 0, 126);
  display.fillRect(1, 36, breite, 8, SSD1306_WHITE);
}

void zeichneGrosszahlSeite(Seite &s) {
  display.setTextSize(3);
  display.setCursor(10, 25);
  display.print(s.wert);
  display.setTextSize(1);
}

void zeichneTextSeite(Seite &s) {
  display.setCursor(0, 20);
  display.println("System OK");
  display.println("Keine Fehler");
}
