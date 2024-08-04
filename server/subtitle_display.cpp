#include <iostream>
#include <string>
#include <thread>
#include <fstream>
#include <sstream>
#include <chrono>
#include <SerialStream.h>

// //ran with: 
// //g++ -I/usr/include/libserial subtitle_display.cpp -lserial
LibSerial::SerialStream arduino;

void display(double duration, const std::string& subtitle) {
    arduino.Open("/dev/ttyACM0");
    if (!arduino.IsOpen()) {
        std::cout << "Could not connect to arduino serial port";
        return;
    }
    arduino.SetBaudRate(LibSerial::BaudRate::BAUD_9600);
    char str[100];
    for (int i = 0; i < 100; ++i) {
        if (i == subtitle.size()) {
            str[i] = '\0';
            break;
        }
        str[i] = subtitle[i];
    }
    str[99] = '\0';
    /*
    std::cout << str << std::endl;
    */
    arduino << str;
    arduino.Close();
    std::chrono::milliseconds x((int) (duration*1000));
    std::this_thread::sleep_for(x);
}

int main() {
    std::fstream infile("subtitles.txt");
    if (!infile.is_open()) {
        std::cerr << "Could not open subtitles.txt";
        return 1;
    }
    std::string line;
    auto start_time = std::chrono::steady_clock::now();
    while (getline(infile, line)) {
        double start, duration;
        std::string subtitle;
        std::stringstream sstr(line);
        sstr >> start >> duration;
        getline(sstr, subtitle);
        subtitle = subtitle.substr(1, subtitle.size()-1);

        auto current_time = std::chrono::steady_clock::now();
        double elapsed_time = (double) std::chrono::duration_cast<std::chrono::milliseconds>(current_time - start_time).count(); 
        double diff = start - elapsed_time / 1000;
        if (diff > 0) {
            std::chrono::milliseconds x(static_cast<int>(diff*1000));
            std::this_thread::sleep_for(x);
        }
        display(duration, subtitle);
    }
    std::cout << "FINISHED\n";
    return 0;
}