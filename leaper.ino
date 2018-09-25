
/* leaper
  Part of the World Weather Wayback machine project

  by Raymond Blum raygeeknyc@

  http://www.github.com/raygeeknyc/leaper
*/

#include <Servo.h>
#include <time.h>
#include <ESP8266WiFi.h>

#include <ArduinoJson.h>

// North America
float COORDINATES_NYC [] = {40.7, -74.0};
float COORDINATES_SASKATCHEWAN [] = {52.94, -106.45};
// Africa
float COORDINATES_YAOUNDE [] = {3.84, 11.5};
float COORDINATES_GABARONE [] = { -24.6, 25.9};
// South America
float COORDINATES_SANTIAGO [] = { -33.45, -70.67};
float COORDINATES_GEORGETOWN [] = {6.8, -58.156};
// Europe
float COORDINATES_BUCHAREST [] = {44.4, 26.1};
float COORDINATES_DUBLIN [] = {53.35, -6.26};
// Asia
float COORDINATES_SEOUL [] = {37.57, 126.98};
float COORDINATES_CHENNAI [] = {13.08, 80.27};
// Australia and Oceania
float COORDINATES_PERTH [] = { -31.96, 115.86};
float COORDINATES_SUVA [] = {-18.12, 178.45};

// Where this device is homed
float* HOME_COORDINATES = COORDINATES_SUVA;

#define _DEFAULT_POLL_DELAY_MS 5000
int poll_delay;

#define CONNECTION_LED_PIN 2
#define ACTION_LED_PIN LED_BUILTIN

#define SERVO_PIN 14
#define UMBRELLA_CENTER 140
#define UMBRELLA_OPEN 60
#define UMBRELLA_CLOSED 180

#define RAIN_LABEL1 "rain"
#define RAIN_LABEL2 "Rain"
#define SNOW_LABEL1 "snow"
#define SNOW_LABEL2 "Snow"
#define SLEET_LABEL1 "sleet"
#define SLEET_LABEL2 "Sleet"
#define HAIL_LABEL1 "hail"
#define HAIL_LABEL2 "Hail"
#define MOSTLY_CLOUDY_LABEL1 "Mostly Cloudy"
#define MOSTLY_CLOUDY_LABEL2 "Overcast"

#define WEATHER_CLEAR 0
#define WEATHER_CLOUDY 1
#define WEATHER_RAINING 2

#define WEATHER_CLEAR_THRESHOLD 0.2
#define WEATHER_RAINING_THRESHOLD 0.7

const char* wifi_ssid_mfny     = "Google";
const char* wifi_password_mfny = SETME;
const char* wifi_ssid_backup     = "thetardis";
const char* wifi_password_backup = SETME;

const char* wifi_ssid     = wifi_ssid_mfny;
const char* wifi_password = wifi_password_mfny;

const int HTTPS_PORT = 443;
const char* ziggy_host = "ziggy-214721.appspot.com";
const char* weather_host = "api.darksky.net";
const char* DATETIME_LABEL = "datetime=";
const char* DELAY_LABEL = "delay=";

long target;
int weather_tier;

Servo myServo;
int currentServoPosition;

const char* WEATHER_SERVICE = "https://api.darksky.net/forecast/";
const char* WEATHER_PARAMS = "lang=en&units=si&exclude=minutely,hourly,daily,alerts,flags";
const char* WEATHER_API_KEY = SETME:
const char* WEATHER_RESPONSE_FIELD_LABEL = "precipProbability";

void SetActionLEDOn()
{
  digitalWrite(ACTION_LED_PIN, false);
}

void SetActionLEDOff()
{
  digitalWrite(ACTION_LED_PIN, true);
}

void SetConnectionLEDOn()
{
  digitalWrite(CONNECTION_LED_PIN, false);
}

void SetConnectionLEDOff()
{
  digitalWrite(CONNECTION_LED_PIN, true);
}

void MoveServoToPosition(int position, int speed)
{
  if (position < currentServoPosition)
  {
    for (int i = currentServoPosition; i > position; i--)
    {
      myServo.write(i);
      delay(speed);
    }
  }
  else if (position > currentServoPosition)
  {
    for (int i = currentServoPosition; i < position; i++)
    {
      myServo.write(i);
      delay(speed);
    }
  }
  currentServoPosition = position;
}

void setup() {
  Serial.begin(115200);
  Serial.print("in setup(): ");

  target = 0L;
  poll_delay = _DEFAULT_POLL_DELAY_MS;
  pinMode(CONNECTION_LED_PIN, OUTPUT);
  pinMode(ACTION_LED_PIN, OUTPUT);
  myServo.attach(SERVO_PIN);
  MoveServoToPosition(UMBRELLA_CLOSED, 10);
  delay(500);
  MoveServoToPosition(UMBRELLA_OPEN, 10);
  delay(500);
  MoveServoToPosition(UMBRELLA_CENTER, 10);
  SetActionLEDOff();
  SetConnectionLEDOff();

  int delay_ms = getDelay();
  if (delay_ms > 0) {
    poll_delay = delay_ms;
  }
}

char* getWeatherSummary(float COORDINATES[], long target_timestamp) {

  char url[256];
  // Example query: https://api.darksky.net/forecast/271428dae50898a27f9af234f1497b19/40.7,-84.0,1296216000?exclude=minutely,hourly,daily,alerts,flags
  sprintf(url, "/forecast/%s/%f,%f,%ld?exclude=minutely,hourly,daily,alerts,flags&units=si",
          WEATHER_API_KEY, COORDINATES[0], COORDINATES[1], target_timestamp);

  return getHttpResponse(weather_host, url);
}

char* getHttpResponse(const char* host, char* url) {
  WiFiClientSecure http_client;

  Serial.println("");
  Serial.println("Connecting to Wifi");
  WiFi.begin(wifi_ssid, wifi_password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  SetConnectionLEDOn();

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  Serial.print("Connecting to host: ");
  Serial.println(host);

  if (!http_client.connect(host, HTTPS_PORT)) {
    Serial.println("connection to host failed");
    return 0L;
  }

  Serial.print("Requesting URL: ");
  Serial.println(url);

  // This will send the request to the server
  http_client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                    "Host: " + host + "\r\n" +
                    "Connection: close\r\n\r\n");

  Serial.println("request sent");
  while (http_client.connected()) {
    String line = http_client.readStringUntil('\n');
    if (line == "\r") {
      Serial.println("headers received");
      break;
    }
  }
  String response_body = http_client.readStringUntil('\n');
  Serial.print("Body: ");
  Serial.println(response_body);
  char* response_buffer = (char*)malloc(response_body.length() + 1);
  strcpy(response_buffer, response_body.c_str());
  return response_buffer;
}

long getTarget() {
  // We now create a URI for the request
  char url[] = "/gettarget";
  Serial.print("Requesting URL: ");
  Serial.println(url);
  char* http_response_body;
  String response = String(http_response_body = getHttpResponse(ziggy_host, url));
  int pos;
  long timestamp = -1L;

  Serial.print("Received datetime response: '");
  Serial.print(response);
  Serial.println("'");
  if (response && (pos = response.indexOf(DATETIME_LABEL) >= 0)) {
    String timestamp_value = response.substring(pos + strlen(DATETIME_LABEL) - 1);
    timestamp = timestamp_value.toInt();
  }
  free(http_response_body );
  return timestamp;
}

int getDelay() {
  // We now create a URI for the request
  char url[] = "/getdelay";
  Serial.print("Requesting URL: ");
  Serial.println(url);
  char* http_response_body;
  String response = String(http_response_body = getHttpResponse(ziggy_host, url));
  int pos;
  int delay_ms = -1L;

  Serial.print("Received delay response: '");
  Serial.print(response);
  Serial.println("'");
  if (response && (pos = response.indexOf(DELAY_LABEL) >= 0)) {
    String delay_value = response.substring(pos + strlen(DELAY_LABEL) - 1);
    delay_ms = delay_value.toInt();
  }
  free(http_response_body );
  return delay_ms;
}

void updateUmbrella(int weather_tier) {
  SetActionLEDOn();
  Serial.print("Opening umbrella for tier: ");
  Serial.println(weather_tier);
  switch (weather_tier) {
    case WEATHER_CLEAR:
      MoveServoToPosition(UMBRELLA_CLOSED, 10);
      break;
      ;;
    case WEATHER_CLOUDY:
      MoveServoToPosition(UMBRELLA_CENTER, 10);
      break;
      ;;
    case WEATHER_RAINING:
      MoveServoToPosition(UMBRELLA_OPEN, 10);
      break;
      ;;
  }
  delay(500);
  SetActionLEDOff();
}

int getTierFor(float rain_likelihood) {
  if (rain_likelihood <= WEATHER_CLEAR_THRESHOLD) {
    return WEATHER_CLEAR;
  } else if (rain_likelihood < WEATHER_RAINING_THRESHOLD) {
    return WEATHER_CLOUDY;
  } else {
    return WEATHER_RAINING;
  }
}

int getPrecipitationFor(float COORDINATES[], long target) {

  char* weather_service_response = getWeatherSummary(COORDINATES, target);

  Serial.println(weather_service_response);

  int tier = WEATHER_CLEAR;

  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(weather_service_response);
  free(weather_service_response);

  if (!root.success()) {
    Serial.println("parseObject() failed");
    return 0L;
  }
  float probability = root["currently"]["precipProbability"];
  if (probability) {
    Serial.print("Probability: ");
    Serial.println(probability);
    tier = getTierFor(probability);

    return tier;
  }
  const char* icon = root["currently"]["icon"];
  String icon_upper = String(icon);

  if (icon && strlen(icon)) {
    // if icon contains any of the precip words - return the rainy tier
    Serial.print("Icon: ");
    Serial.println(icon);
    if (strstr(icon, RAIN_LABEL1) || strstr(icon, SNOW_LABEL1) || strstr(icon, SLEET_LABEL1) || strstr(icon, HAIL_LABEL1) ||
        strstr(icon, RAIN_LABEL2) || strstr(icon, SNOW_LABEL2) || strstr(icon, SLEET_LABEL2) || strstr(icon, HAIL_LABEL2)) {
      return WEATHER_RAINING;
    }
  }
  const char* summary = root["currently"]["summary"];
  if (summary && strlen(summary)) {
    Serial.print("Summary: ");
    Serial.println(summary);
    if (strstr(summary, RAIN_LABEL1) || strstr(summary, SNOW_LABEL1) || strstr(summary, SLEET_LABEL1) || strstr(summary, HAIL_LABEL1) ||
        strstr(summary, RAIN_LABEL2) || strstr(summary, SNOW_LABEL2) || strstr(summary, SLEET_LABEL2) || strstr(summary, HAIL_LABEL2)) {
      return WEATHER_RAINING;
    }
    if (strstr(summary, MOSTLY_CLOUDY_LABEL1) || strstr(summary, MOSTLY_CLOUDY_LABEL2)) {
      return WEATHER_CLOUDY;
    }
  }
  return tier;
}

void loop() {
  long new_target = getTarget();

  if (new_target != target) {
    Serial.print("New target date: ");
    Serial.println(new_target);
    int new_weather_tier = getPrecipitationFor(HOME_COORDINATES, new_target);
    Serial.print("Tier: ");
    Serial.println(new_weather_tier);
    if (new_weather_tier != weather_tier) {
      weather_tier = new_weather_tier;
      updateUmbrella(weather_tier);
    }
    target = new_target;
  }
  delay(poll_delay);
}
