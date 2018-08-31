#include <Servo.h>
#include <time.h>
#include <ESP8266WiFi.h>

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

WiFiClient client;
const char* ssid     = "thetardis";
const char* password = "100cloudy";
const char* host = "appengine.com/ziggy";
const char* DATETIME_LABEL = "datetime=";

long target;
int weather_tier;

Servo myServo;
int currentServoPosition;

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
  MoveServoToPosition(UMBRELLA_CENTER, 10); // Initialize

  SetActionLEDOff();
  SetConnectionLEDOff();  
  Serial.begin(115200); 
  ConnectToWifi();
}

long getTarget() {

  // We now create a URI for the request
  String url = "/get?";
  Serial.print("Requesting URL: ");
  Serial.println(url);
  
  // This will send the request to the server
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" + 
               "Connection: close\r\n\r\n");

  while(client.available()){
    String line = client.readStringUntil('\r');
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
  
  // Use WiFiClient class to create TCP connections
  const int httpPort = 80;
  if (!client.connect(host, httpPort)) {
    Serial.println("connection failed");
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
    return 0.10;  
}

void loop() {
  long new_target = getTarget();

  if (new_target != target) {
    Serial.print("New target date");
    float rain_likelihood = getPrecipitationFor(COORDINATES, target);
    int new_weather_tier = getTierFor(rain_likelihood);
    if (new_weather_tier != weather_tier) {
      weather_tier = new_weather_tier;
      updateUmbrella(weather_tier);
    }
  }
}
