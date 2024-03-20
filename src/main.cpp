#include <Arduino.h>
#include <WiFi.h>
#include <time.h>
#include <ZMPT101B.h>
#include <Firebase_ESP_Client.h>
#include <addons/TokenHelper.h>

// Define WiFi Credentials
#define WIFI_SSID "Pixel"
#define WIFI_PASSWORD ""

// Define Firebase Config
#define FIREBASE_API_KEY "AIzaSyAUjDxrp1bMOlPT5k7GziPS8YPywBckVqY"
#define FIREBASE_PROJECT_ID "monitoring-apps-94a1e"
#define FIREBASE_DATABASE_URL "https://monitoring-apps-94a1e-default-rtdb.firebaseio.com/"

// Define User Credentials
#define USER_EMAIL "esp32firebase@gmail.com"
#define USER_PASSWORD "esp32firebase123"

// Define NTP Client to get time
const char* ntpServer = "id.pool.ntp.org";

// Define Firebase Data Object, Authentication, and Config
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
FirebaseJson json;

// Define global variables
#define LED 2
#define ZMPT101B_PIN 35
#define ACS712_PIN 34
#define ZMPTSensitivity 726.75f
String dataLivePath = "/DATA_LIVE";
String dataLoggingPath = "/DATA_LOGGING/";
String dataLoggingCurrent;
String dataLoggingVoltage;
String dataLiveCurrent;
String dataLiveVoltage;
String dataLiveWattage;
String currentPath;
String voltagePath;
String wattagePath;

ZMPT101B voltageSensor(ZMPT101B_PIN, 50.0);
unsigned long startTime = 0;
unsigned long currentTime = 0;
float BaseVol = analogRead(ACS712_PIN) * (3.3 / 4095.0);

//get time function
String getTime(const char* options) {
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return String(0);
  }
  time(&now);
  if (strstr(options, "day")) {
    return String(timeinfo.tm_mday);
  } else if (strstr(options, "month")) {
    return String(timeinfo.tm_mon + 1);
  } else if (strstr(options, "year")) {
    return String(timeinfo.tm_year + 1900);
  } else {
    return String(now);
  }
}

float getCurrentVoltage() {
  float ACSValue = 0.0, Samples = 0.0, AvgACS = 0.0;

  for (int x = 0; x < 500; x++) {
    ACSValue = analogRead(ACS712_PIN);
    Samples = Samples + ACSValue;
    delay (3);
  }
  
  AvgACS = Samples/500;
  float current = fabsf((((AvgACS) * (3.3 / 4095.0)) - BaseVol) / 0.185);

  if (current < 0.09) {
    current = 0;
  }
  return current;
}

float getACVoltage() {
  voltageSensor.setSensitivity(ZMPTSensitivity);
  return voltageSensor.getRmsVoltage();
}

void setup() {
  Serial.begin(115200);
  pinMode(LED, OUTPUT);

  /* Connecting WiFi starts */
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi...");
  
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(400);
  }

  configTime(7 * 3600, 0, ntpServer, "pool.ntp.org");

  Serial.println();
  Serial.print("Connected with local IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  digitalWrite(LED, HIGH);
  /* Connecting WiFi end */

  /* Assign the API key */
  config.api_key = FIREBASE_API_KEY;

  /* Assign user credentials */
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  /* Assign the RTDB URL */
  config.database_url = FIREBASE_DATABASE_URL;

  /* Token Callback function */
  config.token_status_callback = tokenStatusCallback;

  /* Reconnecting */
  Firebase.reconnectWiFi(true);
  fbdo.setResponseSize(4096);

  /* Initialize Firebase */
  Firebase.begin(&config, &auth);

  /* Initialize start time */
  startTime = strtoul(getTime("now").c_str(), NULL, 10);
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    digitalWrite(LED, LOW);
  }
  else {
    digitalWrite(LED, HIGH);

    /* Initialize local variables */
    currentTime = strtoul(getTime("now").c_str(), NULL, 10);

    float current = getCurrentVoltage();
    float voltage = getACVoltage();
    float wattage = current * voltage;

    String year = getTime("year");
    String month = getTime("month");
    String day = getTime("day");

  Serial.println(analogRead(ACS712_PIN));
  Serial.println(voltage);
  Serial.println(wattage);

    /* Set JSON object */
    json.set("/current", current);
    json.set("/voltage", voltage);
    json.set("/wattage", wattage);

    /* Asyncronous write to Firebase */
    if (current && voltage && wattage && currentTime && year && month && day && Firebase.ready()) { 
        if (currentTime >= startTime + 3600) {
            startTime = strtoul(getTime("now").c_str(), NULL, 10) + 3600;
            Firebase.RTDB.setJSON(&fbdo, dataLoggingPath + year + "/" + month + "/" + day + "/" + currentTime, &json);
        }

    /* Write data live to Firebase */
    Firebase.RTDB.setJSON(&fbdo, dataLivePath, &json);
    }  
  }
}
  



