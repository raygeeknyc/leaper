#include <Servo.h>
#include <time.h>
#include <ESP8266WiFi.h>

#include <ArduinoJson.h>

long COORDINATES[] = {1L, 2L};

#define CONNECTION_LED_PIN 2
#define ACTION_LED_PIN LED_BUILTIN

#define SERVO_PIN 14
#define UMBRELLA_CENTER 140
#define UMBRELLA_OPEN 60
#define UMBRELLA_CLOSED 180

#define WEATHER_CLEAR 0
#define WEATHER_CLOUDY 1
#define WEATHER_RAINING 2

#define WEATHER_CLEAR_THRESHOLD 0.2
#define WEATHER_RAINING_THRESHOLD 0.7

WiFiClient ziggy_client, weather_client;
const char* ssid     = "thetardis";
const char* password = "100cloudy";
const char* ziggy_host = "appengine.com/ziggy";
const char* weather_host = "api.darksky.net";
const char* DATETIME_LABEL = "datetime=";

long target;
int weather_tier;

Servo myServo;
int currentServoPosition;

const char* WEATHER_SERVICE = "https://api.darksky.net/forecast/";
const char* WEATHER_PARAMS = "lang=en&units=si&exclude=minutely,hourly,daily,alerts,flags";
const char* WEATHER_API_KEY = "KEEP_DREAMING";
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
  if(position < currentServoPosition)
  {
    for(int i = currentServoPosition; i > position; i--)
    {
      myServo.write(i);
      delay(speed);
    }
  }
  else if(position > currentServoPosition)
  {
    for(int i = currentServoPosition; i < position; i++)
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
  ConnectToWifi();
}

char* getWeatherSummary(long COORDINATES[], long target) {

  // https://api.darksky.net/forecast/271428dae50898a27f9af234f1497b19/40.7,-84.0,1296216000?exclude=minutely,hourly,daily,alerts,flags
  String url = "/forecast/";
  Serial.print("Requesting URL: ");
  Serial.println(url);
  
  // This will send the request to the server
  weather_client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + ziggy_host + "\r\n" + 
               "Connection: close\r\n\r\n");

  while(weather_client.available()){
    String line = weather_client.readStringUntil('\r');
    Serial.print(line);
  }
  // query the weather API - return the response as a C string
  return 0L;
}

long getTarget() {
  // We now create a URI for the request
  String url = "/get?";
  Serial.print("Requesting URL: ");
  Serial.println(url);
  
  // This will send the request to the server
  ziggy_client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + ziggy_host + "\r\n" + 
               "Connection: close\r\n\r\n");

  while(ziggy_client.available()){
    String line = ziggy_client.readStringUntil('\r');
    Serial.print(line);
    if (int pos = line.indexOf(DATETIME_LABEL) >= 0) {
      String timestamp_value = line.substring(pos);
      long timestamp = timestamp_value.toInt();
      Serial.print(timestamp);
      
      return timestamp;
    }
  }
  return -1L;
}

void ConnectToWifi() {
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
  
  const int httpPort = 80;
  
  if (!ziggy_client.connect(ziggy_host, httpPort)) {
    Serial.println("connection to ziggy failed");
    return;
  }
  if (!weather_client.connect(weather_host, httpPort)) {
    Serial.println("connection to weather service failed");
    return;
  }
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

float getPrecipitationFor(long COORDINATES[], long target) {
  char* weather_data = getWeatherSummary(COORDINATES, target);

  int tier = WEATHER_CLEAR;
  
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(weather_data);

  if (!root.success()) {
    Serial.println("parseObject() failed");
    return 0L;
  }
  float probability = root["precipProbability"];
  if (probability) {
    tier = getTierFor(probability);
  }
  const char* icon = root["icon"];
  if (icon && strlen(icon)) {
    // if icon contains any of the precip words - return the rainy tier
  }
  const char* summary = root["summary"];
  if (summary && strlen(summary)) {
    // if summary contains any of the precip words - return the rainy tier
  }
  return tier;
}

void loop() {
  long new_target = getTarget();

  if (new_target != target) {
    Serial.print("New target date");
    int new_weather_tier = getPrecipitationFor(COORDINATES, target);
     if (new_weather_tier != weather_tier) {
      weather_tier = new_weather_tier;
      updateUmbrella(weather_tier);
    }
  }
}
