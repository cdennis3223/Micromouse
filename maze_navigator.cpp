/*
Currently this code is in a debug state as it will output a video to a window showing the edge detection mechanism and numbers
while it moves through the maze.
*/

#include <iostream>
#include <opencv2/opencv.hpp>
#include <stdexcept>
#include <thread>
#include <chrono>
#include <cstdlib>
#include <vector>
#include <fstream>
#include <pigpio.h>

//define all needed gpio pins.
//Define which GPIO Pins are used for motors.
#define RencoderA 5 //physical pin 29 
#define RencoderB 6 //physical pin 31
#define LencoderA 17//physical pin 11 
#define LencoderB 27 //physical pin 13
#define RM1 16 //physical pin 36
#define RM2 19 //physical pin 35
#define LM1 13 //physical pin 33
#define LM2 12 //physical pin 32

//define start button GPIO pin.
#define START_BUTTON_PIN 26 // physical pin 37

//define which GPIO Pins are used for IR.
#define IR1_LED_PIN  9 // physical pin 21
#define IR2_LED_PIN  11 // physical pin 23
#define IR3_LED_PIN  22 // physical pin 15
#define IR4_LED_PIN  25 // physical pin 22
#define IR1_SENSOR_PIN 18  // physical pin 12
#define IR2_SENSOR_PIN  23 // physical pin 16
#define IR3_SENSOR_PIN  24 // physical pin 18
#define IR4_SENSOR_PIN  21 // physical pin 40

using namespace std;
using namespace cv;

void simplifyPath(vector<char>& path);
void Forward();
void turnAround();
void stop();
void turnRight();
void turnLeft();
void setup();
void turnOnIRLED();
void turnOffIRLED();

int main() {

    system("sudo killall pigpiod 2>/dev/null"); // Stop any running pigpiod process
    this_thread::sleep_for(std::chrono::seconds(1)); // Wait for a second to ensure the process is stopped
    system("sudo fuser -k 8888/tcp"); // Kill any process using port 8888

    // Initialize pigpio library
    setup();

    //Wait for the start button to be pressed
    while (gpioRead(START_BUTTON_PIN) == 1) {
        this_thread::sleep_for(std::chrono::milliseconds(100));//allows for debouncing
    }
    cout << "Button pressed, starting navigation..." << endl;

    // Set up camera
    system("libcamera-vid -t 0 --width 640 --height 480 --framerate 30 --inline -n -o udp://127.0.0.1:5000 &");

    this_thread::sleep_for(std::chrono::seconds(2)); // allow stream to start

    string pipeline =
      "udpsrc port=5000 buffer-size=600000 ! "
      "h264parse ! avdec_h264 ! videoconvert ! appsink sync=false max-buffers=1 drop=true";

    VideoCapture cap(pipeline,CAP_GSTREAMER);
    
    if(!cap.isOpened()){
        cerr << "Failed to open camera stream" << endl;
        return -1;
    }

    turnOnIRLED();

    int edgeThresh = 50;
    vector<char> path;//for future run navigation.
    ifstream inPath("path_log.txt");
    int IRSensor1 = 0;
    int IRSensor2 = 0;
    int IRSensor3 = 0;
    int IRSensor4 = 0;
    bool running = true;

    // Check if the file exists and read the path
    if (inPath) {
        string pathC;
        inPath >> pathC;
        cout << "Previous path: " << pathC << endl;
        // Convert the string to a vector of characters
        for (char move : pathC) {
            path.push_back(move);
        }
    } else {
        cout << "No previous path found." << endl;
    }

    //follow the previously saved and simplified path.
    for (char move : pathC) {
        switch (move) {
            case 'F':
                Forward();
                break;
            case 'L':
                turnLeft();
                break;
            case 'R':
                turnRight();
                break;
            case 'B':
                turnAround();
                break;
            }
    }

    namedWindow("Maze Navigator", WINDOW_AUTOSIZE);

    while(true){

        // Check if the start button is pressed to stop the program
        if (gpioRead(START_BUTTON_PIN) == 0) {
            cout << "Stopping navigation..." << endl;
            running = false;
            break;
        }

        Mat frame, gray, blurred, edges;
        cap >> frame;
        if (frame.empty()){
            break;
        }

        cvtColor(frame,gray, COLOR_BGR2GRAY);
        GaussianBlur(gray, blurred, Size(5,5),1.5);
        Canny(blurred, edges, 50, 150);

        int rows = edges.rows;
        int cols = edges.cols;
        Rect leftROI(0, rows/2, cols/3, rows/2);
        Rect centerROI(cols/3, rows/2, cols/3, rows/2);
        Rect rightROI(2*cols/3, rows/2, cols/3, rows/2);

        int leftEdges = countNonZero(edges(leftROI));
        int centerEdges = countNonZero(edges(centerROI));
        int rightEdges = countNonZero(edges(rightROI));

        IRSensor1 = gpioRead(IR1_SENSOR_PIN);
        IRSensor2 = gpioRead(IR2_SENSOR_PIN);
        IRSensor3 = gpioRead(IR3_SENSOR_PIN);
        IRSensor4 = gpioRead(IR4_SENSOR_PIN);

        //decision logic
        string decision;
        if (centerEdges < edgeThresh && IRSensor2 == 0 && IRSensor3 == 0) {
            decision = "Move Frward";
            path.push_back('F');
            Forward();
            stop();
        } else if(leftEdges < edgeThresh && IRSensor1 == 0){
            decision = "Turn Left";
            path.push_back('L');
            turnLeft();
            stop();
        } else if(rightEdges < edgeThresh && IRSensor4 == 0){
            decision = "Turn Right";
            path.push_back('R');
            turnRight();
            stop();
        } else {
            decision = "Stop(Dead End)";
            path.push_back('B');
            turnAround();
            stop();
        }

        //Draw ROI boxes and decision
        rectangle(frame, leftROI, Scalar(255,0,0), 2);
        rectangle(frame, centerROI, Scalar(0,255,0), 2);
        rectangle(frame, rightROI, Scalar(0,0,255), 2);
        putText(frame, decision, Point(20,40), FONT_HERSHEY_SIMPLEX, 1, Scalar(255,255,255), 2);

        imshow("Maze Navigator", frame);
        if (waitKey(10) == 27) break;

    }

    //simplify the path before saving it for the next run.
    simplifyPath(path);

    ofstream outfile("path_log.txt");
    for(char move:path){
        outfile << move;
    }
    outfile << endl;
    outfile.close();

    turnOffIRLED();
    cap.release();
    destroyAllWindows();
    gpioTerminate(); // Cleanup pigpio resources
    return 0;
}

void simplifyPath(vector<char> &path){
    // Optimize the path, if the robot turns left then turns right without going forward that is a backtrack
    // if it goes forward then backtracks that is nothing
    // if it turns left 3 times it should just turn right, and vice versa
    bool changed = true;
    while (changed)
    {
        changed = false;

        for (size_t i = 1; i < path.size(); i++)
        {
            if ((path[i - 1] == 'L' && path[i] == 'R') || (path[i - 1] == 'R' && path[i] == 'L'))
            {
                path[i - 1] = 'B';
                path.erase(path.begin() + i);
                i = max(1, (int)i - 1);
                changed = true;
                break;
            }
        }

        for (size_t i = 1; i < path.size(); i++)
        {
            if ((path[i - 1] == 'F' && path[i] == 'B') || (path[i - 1] == 'B' && path[i] == 'F'))
            {
                path.erase(path.begin() + i - 1, path.begin() + i + 1);
                i = max(1, (int)i - 2);
                changed = true;
                break;
            }
        }

        for (size_t i = 2; i < path.size(); i++)
        {
            if (path[i - 2] == 'L' && path[i - 1] == 'L' && path[i] == 'L')
            {
                path[i - 2] = 'R';
                path.erase(path.begin() + i - 1, path.begin() + i + 1);
                i = max(2, (int)i - 2);
                changed = true;
                break;
            }
        }

        for (size_t i = 2; i < path.size(); i++)
        {
            if (path[i - 2] == 'R' && path[i - 1] == 'R' && path[i] == 'R')
            {
                path[i - 2] = 'L';
                path.erase(path.begin() + i - 1, path.begin() + i + 1);
                i = max(2, (int)i - 2);
                changed = true;
                break;
            }
        }

        for (size_t i = 2; i < path.size(); i++)
        {
            if (path[i - 1] == 'B' && path[i] == 'B')
            {
                path.erase(path.begin() + i - 1, path.begin() + i + 1);
                i = max(2, (int)i - 2);
                changed = true;
                break;
            }
        }
    }
}

//Motor Functions will need to change to turnRight(), turnLeft(), turnAround(), Forward() controlling
//both motors at the same time.
void Forward(){
    gpioWrite(LM1, 1);
    gpioWrite(LM2, 0);
    gpioWrite(RM1, 1);
    gpioWrite(RM2, 0);
    gpioDelay(500*1000);// Run motors forward for 500ms, adjust as needed
}

void turnAround(){
    gpioWrite(LM1, 0);
    gpioWrite(LM2, 1);
    gpioWrite(RM1, 1);
    gpioWrite(RM2, 0);
    //gpioDelay(); Figure out how long it takes to turn 180 degrees. Then stop all motors.
}

void stop(){
    gpioWrite(LM1, 0);
    gpioWrite(LM2, 0);
    gpioWrite(RM1, 0);
    gpioWrite(RM2, 0);
}

void turnRight(){
    gpioWrite(LM1, 1);
    gpioWrite(LM2, 0);
    gpioWrite(RM1, 0);
    gpioWrite(RM2, 1);
    //gpioDelay(); Will test how much time it needs to turn 90 degrees. Then turn them all off.
}

void turnLeft(){
    gpioWrite(LM1, 0);
    gpioWrite(LM2, 1);
    gpioWrite(RM1, 1);
    gpioWrite(RM2, 0);
    //gpioDelay(); some amount of time in ms need to test when we have mouse live. Then turn them all off.
}


//All the IR sensor functions.
void setup() {
    if (gpioInitialise() < 0) {
        cerr << "Failed to initialize pigpio library." << endl;
    }

    //Setup IR Sensors
    gpioSetMode(IR1_LED_PIN, PI_OUTPUT);
    gpioSetMode(IR2_LED_PIN, PI_OUTPUT);
    gpioSetMode(IR3_LED_PIN, PI_OUTPUT);  // Set IR LED pin as output
    gpioSetMode(IR4_LED_PIN, PI_OUTPUT);  // Set IR LED pin as output
    gpioSetMode(IR1_SENSOR_PIN, PI_INPUT);
    gpioSetMode(IR2_SENSOR_PIN, PI_INPUT);
    gpioSetMode(IR3_SENSOR_PIN, PI_INPUT);  // Set IR sensor pin as input
    gpioSetMode(IR4_SENSOR_PIN, PI_INPUT);  // Set IR sensor pin as input

    //setup motors
    gpioSetMode(RM1, PI_OUTPUT);
    gpioSetMode(RM2, PI_OUTPUT);
    gpioSetMode(LM1, PI_OUTPUT);
    gpioSetMode(LM2, PI_OUTPUT);
    gpioSetMode(LencoderA, PI_INPUT);
    gpioSetMode(LencoderB, PI_INPUT);
    gpioSetMode(RencoderA, PI_INPUT);
    gpioSetMode(RencoderB, PI_INPUT);

    //setup start button
    gpioSetMode(START_BUTTON_PIN, PI_INPUT);
    gpioSetPullUpDown(START_BUTTON_PIN, PI_PUD_UP); // Set pull-up resistor
}

void turnOnIRLED() {
    gpioWrite(IR1_LED_PIN, 1);
    gpioWrite(IR2_LED_PIN, 1);
    gpioWrite(IR3_LED_PIN, 1);  // Turn on IR LED
    gpioWrite(IR4_LED_PIN, 1);
}

void turnOffIRLED() {
    gpioWrite(IR1_LED_PIN, 0);
    gpioWrite(IR2_LED_PIN, 0);
    gpioWrite(IR3_LED_PIN, 0);  // Turn off IR LED
    gpioWrite(IR4_LED_PIN, 0);
}
