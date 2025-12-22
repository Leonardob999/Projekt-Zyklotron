#include <Arduino.h>
#include "globals.h"

void displayTask(void *pvParameters);
void relaisTask(void *pvParameters);

void setup() {
  Serial.begin(115200);

  tempoMutex = xSemaphoreCreateMutex();

  xTaskCreatePinnedToCore(
    displayTask,
    "DisplayTask",
    10000,
    NULL,
    2,
    NULL,
    1        // Core 1 → UI
  );

  xTaskCreatePinnedToCore(
    relaisTask,
    "RelaisTask",
    4096,
    NULL,
    1,
    NULL,
    0        // Core 0 → Logik
  );
}

void loop() {
  vTaskDelay(portMAX_DELAY);
}
