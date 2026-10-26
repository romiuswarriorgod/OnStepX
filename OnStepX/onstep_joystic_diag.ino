#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
// add oled
//  Screen configuration (OLED 0.96")
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

// Системні статуси для екрану
String currentStatus = "Booting...";
unsigned long lastDisplayUpdate = 0;

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
// Оновлення екрану (працює миттєво на Core 1)
// --------------------------------------------------
void updateDisplay()
{
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

  // 2. ГРАФІЧНІ СТРІЛКИ ДЖОЙСТИКА (З інверсією та заміною осей для коректного малювання)
  drawArrowUp(displayXDirection == 1);    // Север
  drawArrowDown(displayXDirection == -1); // Юг
  drawArrowLeft(displayYDirection == -1); // Запад
  drawArrowRight(displayYDirection == 1); // Восток

  // 3. СЕРЕДНІЙ РЯДОК: Текстовий статус напрямку руху
  display.setCursor(0, 24);
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

  // Розрахунок часу роботи (Uptime)
  unsigned long totalSeconds = millis() / 1000;
  unsigned long hours = totalSeconds / 3600;
  unsigned long minutes = (totalSeconds % 3600) / 60;
  display.setCursor(0, 40);
  display.printf("Up: %02dh %02dm", hours, minutes);

  // 4. НАЙНИЖЧИЙ РЯДОК: Статус фонового потоку мережі
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
            currentStatus = "WiFi OK | RSSI:" + String(WiFi.RSSI());
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
  analogReadResolution(12);
  analogSetPinAttenuation(JOY_X_PIN, ADC_11db);
  analogSetPinAttenuation(JOY_Y_PIN, ADC_11db);

  // Створюємо чергу для 10 текстових вказівників команд
  httpQueue = xQueueCreate(10, sizeof(char *));

  // Калібруємо центр джойстика
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

  // Запуск фонового асинхронного таска для Wi-Fi/HTTP на ЯДРІ 0
  xTaskCreatePinnedToCore(
      wiFiTask,
      "WiFiTask",
      4096,
      NULL,
      1,
      NULL,
      0);

  currentStatus = "Ready";
  updateDisplay();
}

// --------------------------------------------------
// MAIN LOOP (Крутиться блискавично на Core 1)
// --------------------------------------------------
void loop()
{
  handleSpeedButton();

  // Зчитування та швидка фільтрація джойстика
  int rawX = readFilteredADC(JOY_X_PIN);
  int rawY = readFilteredADC(JOY_Y_PIN);
  filteredX = (filteredX * 3 + rawX) / 4;
  filteredY = (filteredY * 3 + rawY) / 4;
  // Розрахунок напрямків
  int newXDirection = getDirection(filteredX, centerX, displayXDirection, INVERT_X);
  int newYDirection = getDirection(filteredY, centerY, displayYDirection, INVERT_Y);
  int oldDisplayX = displayXDirection;
  int oldDisplayY = displayYDirection;
  displayXDirection = newXDirection;
  displayYDirection = newYDirection;
  // МИТТЄВЕ оновлення графіки на екрані при найменшому русі важеля
  if (displayXDirection != oldDisplayX || displayYDirection != oldDisplayY || (millis() - lastDisplayUpdate > 1000))
  {
    updateDisplay();
    lastDisplayUpdate = millis();
  }
  // Запис команд у чергу FreeRTOS (виконується міттєво)
  setXAxis(newXDirection);
  setYAxis(newYDirection);
  delay(10);
}