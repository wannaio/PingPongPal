// Author: David Tanudin

#include <Servo.h>
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

// Create servo objects
Servo servo_one;
Servo servo_two;
Servo tilt_sensor;

// Define pin assignments
int pir_pin = A0;
int trig_pin_first = 8;
int echo_pin_first = 9;
int trig_pin_second = 3;
int echo_pin_second = 2;

// Variables to store the duration of echo pin pulse and the calculated distance
long duration, distance, ultrasonic_sensor1, ultrasonic_sensor2;

// Variables for calibration and servo operation
int calibration_time = 0;
int servo_position = 60;
bool first_trigger = false;
bool turn_left = true;
bool turn_right = true;
bool trigger_movement = true;

// Create an RF24 radio object
RF24 radio(7, 6);  // CE, CSN

// Setup function runs once when you press reset or power the board
void setup() {

  Serial.begin(9600);  // Start the serial communication with the baud rate of 9600
  // Setup the input and output pins
  pinMode(pir_pin, INPUT);
  pinMode(trig_pin_first, OUTPUT);
  pinMode(echo_pin_first, INPUT);
  pinMode(trig_pin_second, OUTPUT);
  pinMode(echo_pin_second, INPUT);

  // Attach servos to the defined servo objects
  tilt_sensor.attach(10);
  servo_one.attach(5);
  servo_two.attach(4);

  // Initialize the servo position in case human has moved it
  tilt_sensor.write(60);

  // Initialize the RF24 radio
  radio.begin();
  const byte address[6] = "00001";
  radio.openWritingPipe(address);
  radio.openReadingPipe(0, address);
  radio.setPALevel(RF24_PA_MIN);
}

// Function to read the PIR sensor
void PIRSensor() {
  // If PIR sensor detects motion, rotate the servos
  if(analogRead(pir_pin) > 500) {
    first_trigger = true;
    servo_one.write(45);
    servo_two.write(135);
  }
}

// Function to read the ultrasonic sensor
void SonarSensor(int trig_pin_sensor, int echo_pin_sensor) {
  // Send a short pulse
  digitalWrite(trig_pin_sensor, LOW);
  delayMicroseconds(2);
  digitalWrite(trig_pin_sensor, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig_pin_sensor, LOW);

  // Measure the duration of the echo pin pulse
  duration = pulseIn(echo_pin_sensor, HIGH);

  // Calculate the distance based on the duration of the echo pin pulse
  distance = (duration/2) / 29.1;
}

// Main program loop
void loop() {
  // Calibration for human error, getting ready

  while (calibration_time != 10) {
    Serial.print(calibration_time);
    calibration_time += 1;
    delay(1000);
    return;
  }

  radio.stopListening();
  char text[] = "";

  // Check PIR sensor and adjust servos if motion is detected
  if (first_trigger == false) {
    strcat(text, "false");
    PIRSensor();
  }
  else {
    strcat(text, "true");

    // Read the ultrasonic sensors
    SonarSensor(trig_pin_first, echo_pin_first);
    ultrasonic_sensor1 = distance;
    SonarSensor(trig_pin_second, echo_pin_second);
    ultrasonic_sensor2 = distance;

    // Control servos based on sensor readings
    if (ultrasonic_sensor2 > 10 && turn_left == true) {
      turn_left = false;
      turn_right = true;
      servo_one.write(90);
      servo_two.write(90);
      delay(1000);
      servo_one.write(45);
      servo_two.write(135);
    }
    
    if (ultrasonic_sensor1 > 10 && turn_right == true) {
      turn_right = false;
      turn_left = true;
      servo_one.write(90);
      servo_two.write(90);
      delay(1000);
      servo_one.write(135);
      servo_two.write(45);
    }
  }

  // Send the PIR sensor status over the radio
  radio.write(&text, sizeof(text));
  radio.startListening();
  delay(10);
  
  // If data is received over the radio
  if (radio.available()) {
    char received_message[32] = "";
    radio.read(&received_message, sizeof(received_message));
    int value = atoi(received_message);

    // Apply the bandpass filter
    if (value < 60) {
      value = 60;
    }
    else if (value > 120) {
      value = 120;
    }
    tilt_sensor.write(value);
  }
}