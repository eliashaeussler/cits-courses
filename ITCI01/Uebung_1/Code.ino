#include <Adafruit_LiquidCrystal.h>
#include <Servo.h>
#include <Wire.h>

// Components
Adafruit_LiquidCrystal lcd(0);
Servo doorServo;
Servo blindsServo;

// Constants
const int distanceThreshold = 100;
const int temperatureThreshold = 25;
const int lightThresholdOpen = 700;
const int lightThresholdClose = 400;
const int maxDisplayCols = 16;
const int maxDisplayRows = 2;

// Input pins
const int pirPin = 2;
const int doorButtonPin = 3;
const int distanceSensorPin = 4;
const int temperatureSensorPin = A0;
const int lightSensorPin = A1;

// Output pins
const int buzzerPin = 9;
const int ledPin = 10;
const int doorServoPin = 11;
const int fanMotorPin = 12;
const int blindsServoPin = 13;

// States
int lastMotionState = LOW;
int lastDoorButtonState = LOW;
bool lastMotionSensorState = false;
long lastMotionSensorDistance = 0;
bool lastTempSensorState = false;
int doorOpenedBy = 0;
float lastTemperature = 0.0;
bool blindsOpened = false;



void setup()
{
  // Input pins
  pinMode(pirPin, INPUT);
  pinMode(doorButtonPin, INPUT);
  pinMode(distanceSensorPin, INPUT);
  pinMode(temperatureSensorPin, INPUT);
  pinMode(lightSensorPin, INPUT);

  // Output pins
  pinMode(buzzerPin, OUTPUT);
  pinMode(ledPin, OUTPUT);
  pinMode(doorServoPin, OUTPUT);
  pinMode(fanMotorPin, OUTPUT);
  pinMode(blindsServoPin, OUTPUT);

  // Initialize display
  Wire.begin();
  lcd.begin(maxDisplayCols, maxDisplayRows);
  lcd.setBacklight(LOW);

  // Connect door servo
  doorServo.attach(doorServoPin);
  doorServo.write(0);

  // Connect blinds servo
  blindsServo.attach(blindsServoPin);
  blindsServo.write(0);

  // Start serial
  Serial.begin(9600);
  Serial.println("Hello World!");
}



void loop()
{
  // Read state from inputs
  int motionState = digitalRead(pirPin);
  int doorButtonState = digitalRead(doorButtonPin);
  long motionSensorDistance = readSensorDistance();
  bool motionSensorState = motionSensorDistance < distanceThreshold;
  float temperature = readTemperature();
  bool temperatureState = temperature >= temperatureThreshold;
  int lightValue = analogRead(lightSensorPin);

  // Manage alarm
  if (motionState != lastMotionState) {
    if (motionState == HIGH) {
      alarmOn();
    } else {
      alarmOff();
    }
  }

  // Manage doors servo
  if (doorButtonState != lastDoorButtonState) {
    if (doorButtonState == HIGH) {
      openDoor(doorButtonPin);
    } else {
      closeDoor(doorButtonPin);
    }
  }

  // Manage motion sensor
  if (motionSensorState != lastMotionSensorState && motionSensorDistance != lastMotionSensorDistance) {
    if (motionSensorState) {
      objectDetected();
    } else {
      noObjectDetected();
    }
  }

  // Manage fan motor
  if (temperatureState != lastTempSensorState && temperature != lastTemperature) {
    if (temperatureState) {
      highTemperature();
    } else {
      lowTemperature();
    }
  }

  // Manage blinds servo
  if (lightValue >= lightThresholdOpen) {
    openBlinds();
  } else if (lightValue < lightThresholdClose) {
    closeBlinds();
  }

  // Cache states
  lastMotionState = motionState;
  lastDoorButtonState = doorButtonState;
  lastMotionSensorState = motionSensorState;
  lastMotionSensorDistance = motionSensorDistance;
  lastTempSensorState = temperatureState;
  lastTemperature = temperature;

  // Delay for stabilization
  delay(100);
}



long readSensorDistance()
{
  // Reset sensor pin mode
  pinMode(distanceSensorPin, OUTPUT);
  digitalWrite(distanceSensorPin, LOW);
  delayMicroseconds(2);

  // Switch sensor pin mode
  digitalWrite(distanceSensorPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(distanceSensorPin, LOW);
  pinMode(distanceSensorPin, INPUT);

  // Read sound wave travel time in microseconds
  long travelTime = pulseIn(distanceSensorPin, HIGH);

  return travelTime * 0.01723;
}

float readTemperature()
{
  int sensorValue = analogRead(temperatureSensorPin);
  float voltage = sensorValue * (5.0 / 1023.0);

  return (voltage - 0.5) * 100.0;
}

void alarmOn()
{
  Serial.println("Motion detected - Alarm on!");
  printOnDisplay("ALARM ON!");

  digitalWrite(buzzerPin, HIGH);
  digitalWrite(ledPin, HIGH);
}

void alarmOff()
{
  Serial.println("Alarm off.");
  printOnDisplay("ALARM off");

  digitalWrite(buzzerPin, LOW);
  digitalWrite(ledPin, LOW);
}

void objectDetected()
{
  openDoor(distanceSensorPin);

  Serial.println("Object detected!");
}

void noObjectDetected()
{
  closeDoor(distanceSensorPin);

  Serial.println("No more objects in area.");
}

void openDoor(int triggerPin)
{
  // Early return if door is already opened
  if (doorOpenedBy != 0) {
    return;
  }

  Serial.println("Doors are being opened.");
  printOnDisplay("DOORS OPEN!");

  // Save trigger
  doorOpenedBy = triggerPin;

  // Open door
  doorServo.write(90);
}

void closeDoor(int triggerPin)
{
  // Early return if door is already closed
  if (doorOpenedBy == 0) {
    return;
  }

  Serial.println("Doors are being closed.");
  printOnDisplay("DOORS closed");

  // Save state
  doorOpenedBy = 0;

  // Close door
  doorServo.write(0);
}

void highTemperature()
{
  Serial.println("Temperature is high - Starting fan!");
  printOnDisplay("FAN RUNNING!");

  digitalWrite(fanMotorPin, HIGH);
}

void lowTemperature()
{
  Serial.println("Temperature is low.");
  printOnDisplay("FAN off");

  digitalWrite(fanMotorPin, LOW);
}

void openBlinds()
{
  // Early return if blinds are already opened
  if (blindsOpened) {
    return;
  }

  Serial.println("Brightness is high enough - Opening blinds!");
  printOnDisplay("BLINDS OPEN!");

  // Save state
  blindsOpened = true;

  // Open blinds
  blindsServo.write(90);
}

void closeBlinds()
{
  // Early return if blinds are already closed
  if (!blindsOpened) {
    return;
  }

  Serial.println("Brightness is low - Closing blinds!");
  printOnDisplay("BLINDS closed");

  // Save state
  blindsOpened = false;

  // Open blinds
  blindsServo.write(0);
}

void printOnDisplay(String text)
{
  lcd.clear();
  lcd.setBacklight(HIGH);
  lcd.setCursor(0, 0);
  lcd.print(text);
}
