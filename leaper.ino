#include <Servo.h>
#include <time.h>
#include <ESP8266WiFi.h>

#include <ArduinoJson.h>

float COORDINATES_NYC [] = {40.7, -74.0};
float COORDINATES_YAOUNDE [] = {3.84, 11.5};

float* COORDINATES = COORDINATES_NYC;

#define POLL_DELAY 10000

#define CONNECTION_LED_PIN 2
#define ACTION_LED_PIN LED_BUILTIN

#define SERVO_PIN 14
#define UMBRELLA_CENTER 140
#define UMBRELLA_OPEN 60
#define UMBRELLA_CLOSED 180

#define RAIN_LABEL "rain"
#define SNOW_LABEL "snow"
#define SLEET_LABEL "sleet"
#define HAIL_LABEL "hail"

#define WEATHER_CLEAR 0
#define WEATHER_CLOUDY 1
#define WEATHER_RAINING 2

#define WEATHER_CLEAR_THRESHOLD 0.2
#define WEATHER_RAINING_THRESHOLD 0.7

const char* ssid     = "thetardis";
const char* password = "100cloudy";
const int HTTPS_PORT = 443;
const char* ziggy_host = "ziggy-214721.appspot.com";
const char* weather_host = "api.darksky.net";
const char* DATETIME_LABEL = "datetime=";

long target;
int weather_tier;

Servo myServo;
int currentServoPosition;

const char* WEATHER_SERVICE = "https://api.darksky.net/forecast/";
const char* WEATHER_PARAMS = "lang=en&units=si&exclude=minutely,hourly,daily,alerts,flags";
const char* WEATHER_API_KEY = "271428dae50898a27f9af234f1497b19";
const char* WEATHER_RESPONSE_OBJECT_LABEL = "currently";
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
  target = 0L;
  weather_tier = 0;
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
  Serial.begin(115200);
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
  WiFi.begin(ssid, password);
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
    Serial.print("Header: ");
    Serial.println(line);
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

  String response = String(getHttpResponse(ziggy_host, url));
  int pos;

  Serial.print("Received datetime response: '");
  Serial.print(response);
  Serial.println("'");
   if (response && (pos = response.indexOf(DATETIME_LABEL) >= 0)) {  // This assumes that the datetime value is the end of the response
    String timestamp_value = response.substring(pos+strlen(DATETIME_LABEL)-1);
    long timestamp = timestamp_value.toInt();
    return timestamp;
  }
  return -1L;
}

void updateUmbrella(int weather_tier) {
  SetActionLEDOn();
  switch (weather_tier) {
    case WEATHER_CLEAR:
      MoveServoToPosition(UMBRELLA_CLOSED, 10);
      ;;
    case WEATHER_CLOUDY:
      MoveServoToPosition(UMBRELLA_CENTER, 10);
      ;;
default: WEATHER_RAIN:
      MoveServoToPosition(UMBRELLA_OPEN, 10);
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

float getPrecipitationFor(float COORDINATES[], long target) {
  
  char* weather_data = getWeatherSummary(COORDINATES, target);

  Serial.println(weather_data);
  
  int tier = WEATHER_CLEAR;

  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(weather_data);

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
  if (icon && strlen(icon)) {
    // if icon contains any of the precip words - return the rainy tier
    Serial.print("Icon: ");
    Serial.println(icon);
    if (strstr(icon, RAIN_LABEL) || strstr(icon, SNOW_LABEL) || strstr(icon, SLEET_LABEL) || strstr(icon, HAIL_LABEL)) {
      return WEATHER_RAINING;
    }
  }
  const char* summary = root["currently"]["summary"];
  if (summary && strlen(summary)) {
    Serial.print("Summary: ");
    Serial.println(summary);
    if (strstr(summary, RAIN_LABEL) || strstr(summary, SNOW_LABEL) || strstr(summary, SLEET_LABEL) || strstr(summary, HAIL_LABEL)) {
      return WEATHER_RAINING;
    }
  }
  return tier;
}

void loop() {
  long new_target = getTarget();

  if (new_target != target) {
    Serial.print("New target date: ");
    Serial.println(new_target);
    int new_weather_tier = getPrecipitationFor(COORDINATES, new_target);
    Serial.print("Tier: ");
    Serial.println(new_weather_tier);
    if (new_weather_tier != weather_tier) {
      weather_tier = new_weather_tier;
      updateUmbrella(weather_tier);
    }
    target = new_target;
  }
  delay(POLL_DELAY);
}
