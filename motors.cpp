/*
This will test each motor individually.
*/
#include <iostream>
#include <pigpio.h>
#include <cstdlib>
#include <chrono>
#include <thread>

//Define which GPIO Pins are used for motors.
#define RencoderA 5 //physical pin 29 
#define RencoderB 6 //physical pin 31
#define LencoderA 17//physical pin 11 
#define LencoderB 27 //physical pin 13
#define RM1 16 //physical pin 36 
#define RM2 19 //physical pin 35
#define LM1 13 //physical pin 33
#define LM2 12 //physical pin 32

using namespace std;
void setupPins();
void motorLforward();
void motorLbackward();
void motorLstop();
void motorRforward();
void motorRbackward();
void motorRstop();

void setupPins(){

    system("sudo killall pigpiod")//kills all daemon as this code doesnt use the daemon
    this_thread::sleep_for(std::chrono::seconds(1));
    system("sudo fuser -k 8888/tcp");

    if(gpioInitialise() < 1){
        cerr << "GPIO Initialization Failed" << endl;
        return 1;
    }

    gpioSetMode(RM1, PI_OUTPUT);
    gpioSetMode(RM2, PI_OUTPUT);
    gpioSetMode(LM1, PI_OUTPUT);
    gpioSetMode(LM2, PI_OUTPUT);
    gpioSetMode(LencoderA, PI_INPUT);
    gpioSetMode(LencoderB, PI_INPUT);
    gpioSetMode(RencoderA, PI_INPUT);
    gpioSetMode(RencoderB, PI_INPUT);
}

void motorLforward(){
    gpioWrite(LM1, 1);
    gpioWrite(LM2, 0);
}

void motorLbackward(){
    gpioWrite(LM1, 0);
    gpioWrite(LM2, 1);
}

void motorLstop(){
    gpioWrite(LM1, 0);
    gpioWrite(LM2, 0);
}

void motorRforward(){
    gpioWrite(RM1, 1);
    gpioWrite(RM2, 0);
}

void motorRbackward(){
    gpioWrite(RM1, 0);
    gpioWrite(RM2, 1);
}

void motorRstop(){
    gpioWrite(RM1, 0);
    gpioWrite(RM2, 0);
}

int main(){

    if(gpioInitialise() < 0){
        cerr << "Failed to initialize pigpio" << endl;
        return 1;
    }

    setupPins();

    //test each motor control function.

    motorLforward();
    motorRforward();
    gpioDelay(2000000);//gpioDelay is in microseconds

    motorLbackward();
    motorRbackward();
    gpioDelay(2000000);

    motorLforward();
    motorRbackward();
    gpioDelay(2000000);

    motorRforward();
    motorLbackward();
    gpioDelay(2000000);

    motorLstop();
    motorRstop();

    gpioTerminate();
    return 0;
}