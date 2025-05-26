/*
This will simply open a generic video stream and window showing it for Pi model 4b and PiCamera module 3.
*/
#include <opencv2/opencv.hpp>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <chrono>
#include <cstdlib>

//this file opens a stream from the Raspberry Pi camera and displays it using OpenCV.
int main() {
    //Begin the camera stream from teh Raspberry Pi camera
    std::system("libcamera-vid -t 0 --width 640 --height 480 framerate 30 --inline -n -o udp://127.0.0.1:5000 &");
    std::this_thread::sleep_for(std::chrono::seconds(5)); // Wait for stream

    std::string pipeline =
        "udpsrc port=5000 buffer-size=600000 ! "
        "h264parse ! avdec_h264 ! videoconvert ! appsink sync=false max-buffers=1 drop=true";

    std::cout << "Opening GStreamer pipeline..." << std::endl;

    cv::VideoCapture cap(pipeline, cv::CAP_GSTREAMER);

    if (!cap.isOpened()) {
        std::cerr << "Failed to open stream." << std::endl;
        return 1;
    }

    cv::Mat frame;
    while (true) {
        if (!cap.read(frame)) {
            std::cerr << "Failed to read frame." << std::endl;
            continue;
        }

        cv::imshow("Pi Camera Stream", frame);

        if (cv::waitKey(1) == 27) break; // ESC
    }

    cap.release();
    cv::destroyAllWindows();

    std::system("pkill -f libcamera-vid"); // Stop the camera stream
    return 0;
}