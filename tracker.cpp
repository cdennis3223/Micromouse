/*
This code sets up the video stream that will be used for edge detection with a window allowing adjustments for edge detection 
thresholds. Must be ran on RPi with Camera attached and enabled, Designed for Pi module 4b with PiCamera module 3.
*/
#include <iostream>
#include <opencv2/opencv.hpp>
#include <stdexcept>
#include <thread>
#include <chrono>
#include <cstdlib>

using namespace std;
using namespace cv;

int main() {
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

    int lowThresh = 50;
    int highThresh = 150;

    namedWindow("Edge Tracker", WINDOW_AUTOSIZE);

    createTrackbar("Low","Edge Tracker", &lowThresh, 255, nullptr);
    createTrackbar("High", "Edge Tracker", &highThresh, 255, nullptr);

    while (true){
        Mat frame, gray, blurred, edges, display;
        cap >> frame;
        if(frame.empty()){
            break;
        }

        //preprocessing
        cvtColor(frame,gray, COLOR_BGR2GRAY);
        GaussianBlur(gray,blurred, Size(5,5), 1.5);
        Canny(blurred, edges, lowThresh, highThresh); //edge detection

        //draw the window
        vector<vector<Point>> contours;
        findContours(edges.clone(), contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

        display = frame.clone();
        for(const auto& contour: contours){
            if (contourArea(contour) < 100){
                continue;
            }
            Rect box = boundingRect(contour);
            rectangle(display, box, Scalar(0,255,0),2);
        }
        
        imshow("Edges", edges);
        imshow("Edge Tracker", display);
        if(waitKey(1) == 27) break;//press escape to exit
    }

    cap.release();
    destroyAllWindows();
    return 0;
}