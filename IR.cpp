/*
This will Test the IR Sensors, Might want to set up a while loop to constantly read the sensors for initial testing
*/
#include <iostream>
#include <pigpio.h>
#include <vector>
#include <cstdlib>
#include <chrono>
#include <thread>

using namespace std;

//This code is untested but should work to turn on and off the IR LEDs and read the IR sensors. It uses the pigpio library to control GPIO pins on a Raspberry Pi.
#define IR1_LED_PIN  9 // physical pin 21
#define IR2_LED_PIN  11 // physical pin 23
#define IR3_LED_PIN  22 // physical pin 15
#define IR4_LED_PIN  25 // physical pin 22
#define IR1_SENSOR_PIN 18  // physical pin 12
#define IR2_SENSOR_PIN  23 // physical pin 16
#define IR3_SENSOR_PIN  24 // physical pin 18
#define IR4_SENSOR_PIN  21 // physical pin 40

void setup() {

    system("sudo killall pigpiod")//kills all daemon as this code doesnt use the daemon
    this_thread::sleep_for(std::chrono::seconds(1));
    system("sudo fuser -k 8888/tcp");

    if (gpioInitialise() < 0) {
        cerr << "Failed to initialize pigpio library." << endl;
    }

    gpioSetMode(IR1_LED_PIN, PI_OUTPUT);
    gpioSetMode(IR2_LED_PIN, PI_OUTPUT);
    gpioSetMode(IR3_LED_PIN, PI_OUTPUT);  // Set IR LED pin as output
    gpioSetMode(IR4_LED_PIN, PI_OUTPUT);  // Set IR LED pin as output
    gpioSetMode(IR1_SENSOR_PIN, PI_INPUT);
    gpioSetMode(IR2_SENSOR_PIN, PI_INPUT);
    gpioSetMode(IR3_SENSOR_PIN, PI_INPUT);  // Set IR sensor pin as input
    gpioSetMode(IR3_SENSOR_PIN, PI_INPUT);  // Set IR sensor pin as input
}

void turnOnIRLED() {
    gpioWrite(IR1_LED_PIN, 1);
    gpioWrite(IR2_LED_PIN, 1);
    gpioWrite(IR3_LED_PIN, 1);  // Turn on IR LED
    gpioWrite(IR4_LED_PIN, 1);
    cout << "IR LED ON" << endl;
}

void turnOffIRLED() {
    gpioWrite(IR1_LED_PIN, 0);
    gpioWrite(IR2_LED_PIN, 0);
    gpioWrite(IR3_LED_PIN, 0);  // Turn off IR LED
    gpioWrite(IR4_LED_PIN, 0);
}

void readIRSensor(vector<int> currentSensor) {
        currentSensor[0] = gpioRead(IR1_SENSOR_PIN);  // Read IR sensor value
        currentSensor[1] = gpioRead(IR2_SENSOR_PIN);  // Read IR sensor value
        currentSensor[2] = gpioRead(IR3_SENSOR_PIN);  // Read IR sensor value
        currentSensor[3] = gpioRead(IR4_SENSOR_PIN);  // Read IR sensor value
}

int main() {
    setup();

    turnOnIRLED();

    gpioDelay(200* 1000);  // Wait for 1 second

    vector<int> sensorValue = {0,0,0,0};
    
    readIRSensor(sensorValue);

    for(int i=0; i<sensorValue.size();i++){
        cout << "IR Sensor Value: " << sensorValue[i] << endl;
    }

    cout << "Turning off IR LED..." << endl;
    turnOffIRLED();

    gpioTerminate();  // Clean up and terminate pigpio
    return 0;
}