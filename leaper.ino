#include <Servo.h>
#include <time.h>
#include <ESP8266WiFi.h>

#define CONNECTION_LED_PIN 2
#define ACTION_LED_PIN LED_BUILTIN

#define SERVO_PIN 14
#define CENTER_POSITION 90
#define UMBRELLA_OPEN 60
#define UMBRELLA_CLOSED 180


const char* ssid     = "thetardis";
const char* password = KEEPDREAMING;
const char* host = "wifitest.adafruit.com";

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
  pinMode(CONNECTION_LED_PIN, OUTPUT);
  pinMode(ACTION_LED_PIN, OUTPUT);
  myServo.attach(SERVO_PIN);
  MoveServoToPosition(CENTER_POSITION, 10); // Initialize

  SetActionLEDOff();
  SetConnectionLEDOff();  
  Serial.begin(115200); 
  ConnectToWifi();
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
}

void loop() {
  Serial.println("Center");
  SetActionLEDOff();
  MoveServoToPosition(CENTER_POSITION, 10);
  delay(1000);
  Serial.println("Sweep");
  SetActionLEDOn();
  MoveServoToPosition(UMBRELLA_CLOSED, 10);
  delay(100);
  MoveServoToPosition(UMBRELLA_OPEN, 10);
  delay(1000);
}
