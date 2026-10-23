#include <WiFi.h>
#include <HTTPClient.h>

// Wi-Fi OnStepX
const char* WIFI_SSID = "OnStepX";
const char* WIFI_PASSWORD = "password";

// OnStepX web interface
const char* MOUNT_URL = "http://192.168.0.1/mount.htm";

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

// Speed level R0-R9
int currentRate = 5;

// Button state
int buttonReading = HIGH;
int buttonStableState = HIGH;
unsigned long buttonLastChange = 0;

// Wi-Fi reconnect
unsigned long lastWiFiReconnect = 0;

// ADC smoothing
int filteredX = 2048;
int filteredY = 2048;


// --------------------------------------------------
// Send command to OnStepX
// --------------------------------------------------
bool sendCommand(const String& command)
{
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("[HTTP] Wi-Fi not connected");
    return false;
  }

  HTTPClient http;
  String url = String(MOUNT_URL) + "?dr=" + command;

  http.setConnectTimeout(500);
  http.setTimeout(700);
  http.setReuse(false);

  if (!http.begin(url))
  {
    Serial.println("[HTTP] Cannot begin request");
    return false;
  }

  int httpCode = http.GET();
  http.end();

  if (httpCode <= 0)
  {
    Serial.printf("[HTTP] Request failed: %d, command=%s\n",
                  httpCode, command.c_str());
    return false;
  }

  Serial.printf("[HTTP] %s -> %d\n", command.c_str(), httpCode);
  return true;
}


// --------------------------------------------------
// Wi-Fi connection
// --------------------------------------------------
void connectWiFi()
{
  Serial.printf("[WiFi] Connecting to %s\n", WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  unsigned long started = millis();

  while (WiFi.status() != WL_CONNECTED &&
         millis() - started < 15000)
  {
    delay(250);
    Serial.print(".");
  }

  Serial.println();

  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.printf("[WiFi] Connected | RSSI=%d dBm | IP=%s | Channel=%d\n",
                  WiFi.RSSI(),
                  WiFi.localIP().toString().c_str(),
                  WiFi.channel());
  }
  else
  {
    Serial.println("[WiFi] Connection failed");
  }
}


// --------------------------------------------------
// Reconnect Wi-Fi when needed
// --------------------------------------------------
void maintainWiFi()
{
  if (WiFi.status() == WL_CONNECTED)
    return;

  if (millis() - lastWiFiReconnect < WIFI_RECONNECT_MS)
    return;

  lastWiFiReconnect = millis();

  Serial.println("[WiFi] Reconnecting...");
  WiFi.disconnect();
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}


// --------------------------------------------------
// Read and filter ADC
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
// Calibrate joystick center at startup
// Keep joystick untouched during calibration.
// --------------------------------------------------
void calibrateCenter()
{
  Serial.println("[JOY] Calibrating center. Do not touch joystick.");

  long sumX = 0;
  long sumY = 0;

  const int samples = 100;

  for (int i = 0; i < samples; i++)
  {
    sumX += analogRead(JOY_X_PIN);
    sumY += analogRead(JOY_Y_PIN);
    delay(10);
  }

  centerX = sumX / samples;
  centerY = sumY / samples;

  filteredX = centerX;
  filteredY = centerY;

  Serial.printf("[JOY] Center X=%d Y=%d\n", centerX, centerY);
}


// --------------------------------------------------
// Determine direction using dead zone and hysteresis
// Return: -1, 0, +1
// --------------------------------------------------
int getDirection(int value, int center,
                 int previousDirection, bool invert)
{
  int delta = value - center;

  if (invert)
    delta = -delta;

  int threshold = (previousDirection == 0)
                    ? DEADZONE_ENTER
                    : DEADZONE_EXIT;

  if (delta > threshold)
    return 1;

  if (delta < -threshold)
    return -1;

  return 0;
}


// --------------------------------------------------
// X axis: East / West
// --------------------------------------------------
void setXAxis(int newDirection)
{
  if (newDirection == currentXDirection)
    return;

  // Stop previous X movement
  if (currentXDirection == 1)
    sendCommand("e0");

  else if (currentXDirection == -1)
    sendCommand("w0");

  currentXDirection = 0;

  // Start new X movement
  if (newDirection == 1)
  {
    if (sendCommand("e1"))
      currentXDirection = 1;
  }
  else if (newDirection == -1)
  {
    if (sendCommand("w1"))
      currentXDirection = -1;
  }

  Serial.printf("[JOY] X direction: %d\n", currentXDirection);
}


// --------------------------------------------------
// Y axis: North / South
// --------------------------------------------------
void setYAxis(int newDirection)
{
  if (newDirection == currentYDirection)
    return;

  // Stop previous Y movement
  if (currentYDirection == 1)
    sendCommand("n0");

  else if (currentYDirection == -1)
    sendCommand("s0");

  currentYDirection = 0;

  // Start new Y movement
  if (newDirection == 1)
  {
    if (sendCommand("n1"))
      currentYDirection = 1;
  }
  else if (newDirection == -1)
  {
    if (sendCommand("s1"))
      currentYDirection = -1;
  }

  Serial.printf("[JOY] Y direction: %d\n", currentYDirection);
}


// --------------------------------------------------
// Stop all currently active axes
// --------------------------------------------------
void stopAllAxes()
{
  if (currentXDirection == 1)
    sendCommand("e0");
  else if (currentXDirection == -1)
    sendCommand("w0");

  if (currentYDirection == 1)
    sendCommand("n0");
  else if (currentYDirection == -1)
    sendCommand("s0");

  currentXDirection = 0;
  currentYDirection = 0;
}


// --------------------------------------------------
// Change speed with joystick button
// Cycle R0-R9
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

    // Button pressed (INPUT_PULLUP)
    if (buttonStableState == LOW)
    {
      currentRate++;

      if (currentRate > 9)
        currentRate = 0;

      String command = "R" + String(currentRate);

      Serial.printf("[JOY] Speed selection: R%d\n", currentRate);
      sendCommand(command);
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

  pinMode(JOY_BUTTON_PIN, INPUT_PULLUP);

  analogReadResolution(12);
  analogSetPinAttenuation(JOY_X_PIN, ADC_11db);
  analogSetPinAttenuation(JOY_Y_PIN, ADC_11db);

  connectWiFi();

  calibrateCenter();

  Serial.println();
  Serial.println("==================================");
  Serial.println(" OnStepX Joystick Controller");
  Serial.println(" No joystick-based acceleration");
  Serial.println(" Hold = move, center = stop");
  Serial.println(" Button = change rate R0-R9");
  Serial.println("==================================");
}


// --------------------------------------------------
// Main loop
// --------------------------------------------------
void loop()
{
  maintainWiFi();

  if (WiFi.status() != WL_CONNECTED)
  {
    // Avoid leaving motion commands active after Wi-Fi loss.
    // Reconnect and send stop commands once connection returns.
    delay(10);
    return;
  }

  handleSpeedButton();

  int rawX = readFilteredADC(JOY_X_PIN);
  int rawY = readFilteredADC(JOY_Y_PIN);

  // Light smoothing
  filteredX = (filteredX * 3 + rawX) / 4;
  filteredY = (filteredY * 3 + rawY) / 4;

  int newXDirection = getDirection(
    filteredX, centerX, currentXDirection, INVERT_X
  );

  int newYDirection = getDirection(
    filteredY, centerY, currentYDirection, INVERT_Y
  );

  // Axes are controlled independently, allowing diagonal movement.
  setXAxis(newXDirection);
  setYAxis(newYDirection);

  delay(10);
}