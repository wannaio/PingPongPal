// Authors: wannaio, tom-bou

#include <Wire.h>
#include <Servo.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_ADXL345_U.h>
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

const int MOTOR1_PIN = 5;
const int MOTOR2_PIN = 6;
const int PHOTO_PIN = A0;
const int DELAY_TIME = 1000;
const int LIGHT_THRESHOLD = 60;
const int SERVO_PIN = 10;

Servo myservo;
Adafruit_ADXL345_Unified adxl = Adafruit_ADXL345_Unified();
RF24 radio(7, 8);  // CE, CSN
bool triggered = false;
bool radio_triggered = false;
void setup() {
  Serial.begin(9600);
  Wire.begin();
  if (!adxl.begin()) {
    Serial.println("Could not find a valid ADXL345 sensor, check wiring!");
  }
  pinMode(MOTOR1_PIN, OUTPUT);
  pinMode(MOTOR2_PIN, OUTPUT);
  pinMode(PHOTO_PIN, INPUT);
  myservo.attach(SERVO_PIN);
  radio.begin();
  const byte address[6] = "00001";
  radio.openWritingPipe(address);
  radio.openReadingPipe(1, address);
  radio.setPALevel(RF24_PA_MIN);
  myservo.write(40);
}
void loop() {
  int lightValue = analogRead(PHOTO_PIN);
  Serial.println(lightValue);
  if (lightValue < LIGHT_THRESHOLD) {
    triggered = true;
  }
  if (triggered) {
    digitalWrite(MOTOR1_PIN, HIGH);
    digitalWrite(MOTOR2_PIN, HIGH);
  }
  radio.stopListening();
  // Read acceleration data
  sensors_event_t event;
  adxl.getEvent(&event);
  char buffer[10];
  int servoAngle = map(event.acceleration.x, -10, 10, 0, 180);
  Serial.println(servoAngle);
  itoa(servoAngle, buffer, 10);
  radio.write(&buffer, sizeof(buffer));
  radio.startListening();
  delay(10);
  if (radio.available()) {
    char receivedFlag[32] = "";
    radio.read(&receivedFlag, sizeof(receivedFlag));
    if (strcmp(receivedFlag, "t") == 0) {
      radio_triggered = true;
    }
  }
  if (radio_triggered == true) {
    myservo.write(40);
    delay(2000);
    myservo.write(130);
    delay(200);
  }
}
