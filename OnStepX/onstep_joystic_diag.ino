#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Screen configuration (OLED 0.96")
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Wi-Fi OnStepX
const char *WIFI_SSID = "OnStepX";
const char *WIFI_PASSWORD = "password";

const char *MOUNT_URL = "http://192.168.0.1/mount.htm";

// Joystick pins
const int JOY_X_PIN = 34;
const int JOY_Y_PIN = 35;
const int JOY_BUTTON_PIN = 27;

// Battery voltage pin (ADC1)
const int BATT_PIN = 32;

// Direction inversion
const bool INVERT_X = false;
const bool INVERT_Y = false;

// ADC calibration
int centerX = 2048;
int centerY = 2048;

// Dead zone with hysteresis
const int DEADZONE_ENTER = 1300;
const int DEADZONE_EXIT = 700;

// ADC filtering
const int FILTER_SAMPLES = 8;

// Button debounce
const unsigned long BUTTON_DEBOUNCE_MS = 50;

// Wi-Fi reconnect interval
const unsigned long WIFI_RECONNECT_MS = 5000;

// Current axis directions: -1, 0, +1
int currentXDirection = 0;
int currentYDirection = 0;

// Змінні відображення на екрані
int displayXDirection = 0;
int displayYDirection = 0;

// Speed level R0-R9
int currentRate = 5;

// Button state
int buttonReading = HIGH;
int buttonStableState = HIGH;
unsigned long buttonLastChange = 0;

// ADC smoothing
int filteredX = 2048;
int filteredY = 2048;

// Вольтаж акумулятора
float batteryVoltage = 0.0;

// Системні статуси для екрану
String currentStatus = "Booting...";
unsigned long lastDisplayUpdate = 0;
unsigned long lastBatteryCheck = 0;

// Асинхронна черга FreeRTOS для HTTP команд між ядрами ESP32
QueueHandle_t httpQueue;

// --------------------------------------------------
// Функції малювання контурних та зафарбованих стрілок
// --------------------------------------------------
void drawArrowUp(bool filled)
{
  if (filled)
    display.fillTriangle(105, 14, 97, 24, 113, 24, SSD1306_WHITE);
  else
    display.drawTriangle(105, 14, 97, 24, 113, 24, SSD1306_WHITE);
}

void drawArrowDown(bool filled)
{
  if (filled)
    display.fillTriangle(105, 46, 97, 36, 113, 36, SSD1306_WHITE);
  else
    display.drawTriangle(105, 46, 97, 36, 113, 36, SSD1306_WHITE);
}

void drawArrowLeft(bool filled)
{
  if (filled)
    display.fillTriangle(85, 30, 95, 22, 95, 38, SSD1306_WHITE);
  else
    display.drawTriangle(85, 30, 95, 22, 95, 38, SSD1306_WHITE);
}

void drawArrowRight(bool filled)
{
  if (filled)
    display.fillTriangle(125, 30, 115, 22, 115, 38, SSD1306_WHITE);
  else
    display.drawTriangle(125, 30, 115, 22, 115, 38, SSD1306_WHITE);
}

// --------------------------------------------------
// Функція вимірювання напруги акумулятора 12В
// --------------------------------------------------
// --------------------------------------------------
// Функція вимірювання напруги акумулятора 12В
// --------------------------------------------------
// --------------------------------------------------
// Функція вимірювання напруги акумулятора 12В
// --------------------------------------------------
void checkBattery()
{
  // Пауза для мінімізації впливу радіозавад від Wi-Fi модуля
  delay(2);

  long sum = 0;
  const int samples = 60;
  for (int i = 0; i < samples; i++)
  {
    sum += analogRead(BATT_PIN);
    delayMicroseconds(100);
  }
  float rawAdc = sum / (float)samples;

  // Коефіцієнт відкориговано на 1.421, щоб підняти показник з 9.7В до точних 10.3В
  float pinVoltage = (rawAdc * 3.6 / 4095.0) * 1.421;
  float newVoltage = pinVoltage * ((100.0 + 20.0) / 20.0);

  // Експоненційне згладжування (RC-фільтр) проти імпульсних просідань напруги
  if (batteryVoltage < 1.0)
  {
    batteryVoltage = newVoltage;
  }
  else
  {
    batteryVoltage = (batteryVoltage * 4.0 + newVoltage) / 5.0;
  }
}

// --------------------------------------------------
// Оновлення екрану (працює миттєво на Core 1)
// --------------------------------------------------
void updateDisplay()
{
  // Захист від зсуву екрану через просідання напруги
  display.ssd1306_command(SSD1306_SETSTARTLINE | 0x00);

  display.clearDisplay();

  // 1. ЖОВТА ЗОНА: Прогрес-бар швидкості
  display.drawRect(0, 4, 128, 8, SSD1306_WHITE);
  if (currentRate > 0)
  {
    int barWidth = map(currentRate, 1, 9, 12, 124);
    display.fillRect(2, 6, barWidth, 4, SSD1306_WHITE);
  }

  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // 2. ГРАФІЧНІ СТРІЛКИ ДЖОЙСТИКА
  drawArrowUp(displayXDirection == 1);    // Север
  drawArrowDown(displayXDirection == -1); // Юг
  drawArrowLeft(displayYDirection == -1); // Запад
  drawArrowRight(displayYDirection == 1); // Восток

  // 3. СЕРЕДНІЙ РЯДОК: Текстовий статус напрямку руху
  display.setCursor(0, 20);
  if (displayXDirection == 0 && displayYDirection == 0)
  {
    display.print("Status: Idle");
  }
  else
  {
    display.print("Move: ");
    if (displayXDirection == 1)
      display.print("North");
    else if (displayXDirection == -1)
      display.print("South");

    if (displayXDirection != 0 && displayYDirection != 0)
      display.print("-");

    if (displayYDirection == 1)
      display.print("East");
    else if (displayYDirection == -1)
      display.print("West");
  }

  // 4. РЯДОК НАПРУГИ: Вольтаж акумулятора
  display.setCursor(0, 32);
  display.printf("Batt: %0.1fV", batteryVoltage);
  if (batteryVoltage < 10.5 && batteryVoltage > 3.0)
  {
    display.print("LOW");
  }

  // Розрахунок часу роботи (Uptime)
  unsigned long totalSeconds = millis() / 1000;
  unsigned long hours = totalSeconds / 3600;
  unsigned long minutes = (totalSeconds % 3600) / 60;
  display.setCursor(0, 44);
  display.printf("Up: %02dh %02dm", hours, minutes);

  // 5. НАЙНИЖЧИЙ РЯДОК: Статус фонового потоку мережі
  display.setCursor(0, 56);
  display.print(currentStatus);

  display.display();
}

// --------------------------------------------------
// Асинхронне відправлення команди через чергу FreeRTOS
// --------------------------------------------------
void queueSendCommand(const String &cmd)
{
  char *cmdPtr = strdup(cmd.c_str());
  if (cmdPtr == NULL)
    return;

  if (xQueueSend(httpQueue, &cmdPtr, 0) != pdPASS)
  {
    free(cmdPtr);
  }
}

// --------------------------------------------------
// Зчитування та фільтрація аналогових осей джойстика
// --------------------------------------------------
int readFilteredADC(int pin)
{
  long total = 0;
  for (int i = 0; i < FILTER_SAMPLES; i++)
  {
    total += analogRead(pin);
    delayMicroseconds(300);
  }
  return total / FILTER_SAMPLES;
}

// --------------------------------------------------
// Визначення напрямку (мертва зона + гістерезис)
// --------------------------------------------------
int getDirection(int value, int center, int previousDirection, bool invert)
{
  int delta = value - center;
  if (invert)
    delta = -delta;

  int threshold = (previousDirection == 0) ? DEADZONE_ENTER : DEADZONE_EXIT;

  if (delta > threshold)
    return 1;
  if (delta < -threshold)
    return -1;
  return 0;
}

// --------------------------------------------------
// Логіка зміни осей (Формує команди в асинхронну чергу)
// --------------------------------------------------
void setXAxis(int newDirection)
{
  if (newDirection == currentXDirection)
    return;

  if (currentXDirection == 1)
    queueSendCommand("e0");
  else if (currentXDirection == -1)
    queueSendCommand("w0");

  currentXDirection = 0;

  if (newDirection == 1)
  {
    queueSendCommand("e1");
    currentXDirection = 1;
  }
  else if (newDirection == -1)
  {
    queueSendCommand("w1");
    currentXDirection = -1;
  }
}

void setYAxis(int newDirection)
{
  if (newDirection == currentYDirection)
    return;

  if (currentYDirection == 1)
    queueSendCommand("n0");
  else if (currentYDirection == -1)
    queueSendCommand("s0");

  currentYDirection = 0;

  if (newDirection == 1)
  {
    queueSendCommand("n1");
    currentYDirection = 1;
  }
  else if (newDirection == -1)
  {
    queueSendCommand("s1");
    currentYDirection = -1;
  }
}

// --------------------------------------------------
// ФОНОВИЙ ПОТІК МЕРЕЖІ ТА HTTP (Працює ізольовано на Core 0)
// --------------------------------------------------
void wiFiTask(void *pvParameters)
{
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  unsigned long lastReconnectAttempt = millis();

  while (true)
  {
    if (WiFi.status() != WL_CONNECTED)
    {
      if (millis() - lastReconnectAttempt > WIFI_RECONNECT_MS)
      {
        lastReconnectAttempt = millis();
        WiFi.disconnect();
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
      }
      currentStatus = "WiFi Connecting...";
      vTaskDelay(pdMS_TO_TICKS(100));
      continue;
    }

    char *receivedCmd = NULL;
    if (xQueueReceive(httpQueue, &receivedCmd, pdMS_TO_TICKS(100)) == pdPASS)
    {
      if (receivedCmd != NULL)
      {

        HTTPClient http;
        String url = String(MOUNT_URL) + "?dr=" + String(receivedCmd);
        http.setConnectTimeout(400);
        http.setTimeout(500);
        http.setReuse(false);

        if (http.begin(url))
        {
          int httpCode = http.GET();
          if (httpCode <= 0)
          {
            currentStatus = "HTTP Err: " + String(httpCode);
          }
          else
          {
            currentStatus = "WiFi OK";
          }
          http.end();
        }
        else
        {
          currentStatus = "HTTP Begin Fail";
        }

        free(receivedCmd);
      }
    }
  }
}

// --------------------------------------------------
// Кнопка швидкості
// --------------------------------------------------
void handleSpeedButton()
{
  int reading = digitalRead(JOY_BUTTON_PIN);
  if (reading != buttonReading)
  {
    buttonReading = reading;
    buttonLastChange = millis();
  }

  if (millis() - buttonLastChange < BUTTON_DEBOUNCE_MS)
    return;

  if (reading != buttonStableState)
  {
    buttonStableState = reading;
    if (buttonStableState == LOW)
    {
      currentRate++;
      if (currentRate > 9)
        currentRate = 0;

      updateDisplay();
      queueSendCommand("R" + String(currentRate));
    }
  }
}

// --------------------------------------------------
// Setup
// --------------------------------------------------
void setup()
{
  Serial.begin(115200);
  delay(500);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
  {
    Serial.println("SSD1306 allocation failed");
  }
  else
  {
    display.clearDisplay();
    display.display();
  }
  pinMode(JOY_BUTTON_PIN, INPUT_PULLUP);
  pinMode(BATT_PIN, INPUT);
  analogReadResolution(12);
  analogSetPinAttenuation(JOY_X_PIN, ADC_11db);
  analogSetPinAttenuation(JOY_Y_PIN, ADC_11db);
  analogSetPinAttenuation(BATT_PIN, ADC_11db);
  httpQueue = xQueueCreate(10, sizeof(char *));
  long sumX = 0, sumY = 0;
  for (int i = 0; i < 100; i++)
  {
    sumX += analogRead(JOY_X_PIN);
    sumY += analogRead(JOY_Y_PIN);
    delay(10);
  }
  centerX = sumX / 100;
  centerY = sumY / 100;
  filteredX = centerX;
  filteredY = centerY;
  checkBattery();
  xTaskCreatePinnedToCore(wiFiTask, "WiFiTask", 4096, NULL, 1, NULL, 0);
  currentStatus = "Ready";
  updateDisplay();
}
// --------------------------------------------------
// MAIN LOOP (Крутиться блискавично на Core 1)
// --------------------------------------------------
void loop()
{
  handleSpeedButton();
  int rawX = readFilteredADC(JOY_X_PIN);
  int rawY = readFilteredADC(JOY_Y_PIN);
  filteredX = (filteredX * 3 + rawX) / 4;
  filteredY = (filteredY * 3 + rawY) / 4;
  int newXDirection = getDirection(filteredX, centerX, displayXDirection, INVERT_X);
  int newYDirection = getDirection(filteredY, centerY, displayYDirection, INVERT_Y);
  int oldDisplayX = displayXDirection;
  int oldDisplayY = displayYDirection;
  displayXDirection = newXDirection;
  displayYDirection = newYDirection;
  // Вимірювання напруги раз на 5 секунд
  if (millis() - lastBatteryCheck > 5000)
  {
    checkBattery();
    lastBatteryCheck = millis();
    updateDisplay();
  }
  if (displayXDirection != oldDisplayX || displayYDirection != oldDisplayY || (millis() - lastDisplayUpdate > 1000))
  {
    updateDisplay();
    lastDisplayUpdate = millis();
  }
  setXAxis(newXDirection);
  setYAxis(newYDirection);
  delay(10);
}