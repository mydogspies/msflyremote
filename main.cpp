// msflyremote
// version 0.1
// interfaces Arduino/Nextion with MSFS 2020 via this script. This repo is just a proof of concept and not
// a full working library. No support and no guarantee it works on your setup.
// requires the MSFS2020 SDK and the PMDG SDK

#include <windows.h>
#include <iostream>
#include <ranges>
#include <vector>
#include <string>
#include <thread>
#include <fstream>
#include <cmath>
#include <mutex>
#include <array>
#include <sstream>

using namespace std;
#include "SimConnect.h"

HANDLE  hSimConnect = nullptr;
HANDLE hSerial = nullptr;

std::mutex mtx;

//
// LOGGER DEFINITIONS
//



enum LogLevel {
    LOG_INFO,
    LOG_WARNING,
    LOG_ERROR,
    LOG_LEVELS
};

constexpr std::array<const char*, LOG_LEVELS> levelStrings = {{
    "LOG_INFO",
    "LOG_WARNING",
    "LOG_ERROR"
    }};
const char* levelToString(LogLevel level) {
    if (level >= 0 && level < LOG_LEVELS) {
        return levelStrings[level];
    }
    return "Unknown";
}

enum LogOutput {
    LOG_TO_TERMINAL,
    LOG_TO_FILE
};

// this sets globally if logging writes to terminal or file
LogOutput logOutputTypeSet = LOG_TO_TERMINAL;

//
// FUNCTIONS
//

std::string captureCerrBuffer(std::streambuf* originalCerrBuffer) {

    std::ostringstream cerrStream;
    std::cerr.rdbuf(cerrStream.rdbuf());
    // std::cerr << "This is an error message!" << std::endl;
    std::cerr.rdbuf(originalCerrBuffer);
    return cerrStream.str();
}

// get current time
//
char* getTime() {
    time_t rawtime = time(0);
    struct tm * timeinfo = localtime(&rawtime);
    char* date_time = asctime(timeinfo);
    return date_time;
} // END getTime

// Logging function
// with option to log to terminal or to file
//
void logMessage(LogOutput outputType, LogLevel level, const std::string& message, const std::string& filename = "") {
    std::ostream* outputStream = nullptr;
    std::string msg;

    // Determine the output stream based on the output type
    if (outputType == LOG_TO_FILE) {
        std::ofstream logFile(filename, std::ios::out | std::ios::app);
        if (!logFile.is_open() || !logFile.good()) {
            std::cerr << "Failed to open log file: " << filename << std::endl;
            return;
        }
        outputStream = &logFile;

        switch (level) {
            case LOG_INFO:
                (*outputStream) << getTime() << " | INFO: ";
            break;
            case LOG_WARNING:
                (*outputStream) << getTime() << " | WARNING: ";
            break;
            case LOG_ERROR:
                (*outputStream) << getTime() << " | ERROR: ";
            break;
            default:
                (*outputStream) << getTime() << " | UNKNOWN: ";
        }
        (*outputStream) << message << std::endl;

    } else {

        cout << getTime() << levelToString(level) << ": " << message << endl;
    }
} // END logMessage

//
// MSFS 2020 functions
//

// Test connection to msfs2020
//
void testSimConnect() {

    HRESULT hr;

    if (SUCCEEDED(SimConnect_Open(&hSimConnect, "Open and Close", nullptr, 0, 0, 0))) {

        logMessage(logOutputTypeSet, LOG_INFO, "testSimConnect(): Connected to MSFS 2020.");
        hr = SimConnect_Close(hSimConnect);
        printf("\nDisconnected from Flight Simulator\n");

    } else
        logMessage(logOutputTypeSet, LOG_ERROR, "testSimConnect(): Failed to connect to MSFS 2020");
} // END testSimConnnect


//
// ARDUINO functions
//

// Function to receive data from Arduino
void receiveData() {

    logMessage(logOutputTypeSet, LOG_INFO, "receiveData(): Listening to Arduino.");

    try {
        vector<char> byteStream;
        DWORD bytesRead;
        char byte;

        while (true) {
            if (ReadFile(hSerial, &byte, sizeof(byte), &bytesRead, NULL)) {
                if (bytesRead > 0) {
                    byteStream.push_back(byte);

                    if (byte == '\n') {  // Assuming data ends with newline
                        std::string receivedData(byteStream.begin(), byteStream.end()-1);
                        mtx.lock();
                        cout << "Received: " << receivedData << endl;
                        mtx.unlock();
                        byteStream.clear();
                    }
                }
            } else {
                logMessage(logOutputTypeSet, LOG_ERROR, "receiveData(): Error reading from open serial port.");
            }
        }

    } catch (...) {

        logMessage(logOutputTypeSet, LOG_ERROR, "receiveData(): Error while trying to read serial port.");
    }


}

// Function to send data to Arduino
void sendData(const string& message) {
    try {
        mtx.lock();
        DWORD bytesWritten;
        if (!WriteFile(hSerial, message.c_str(), message.length(), &bytesWritten, NULL)) {
            logMessage(logOutputTypeSet, LOG_ERROR, "sendData(): Error writing data to open port.");
        }
        mtx.unlock();
    } catch (...) {
        logMessage(logOutputTypeSet, LOG_ERROR, "sendData(): Error while trying to send data.");
    }
}

// Test connection to Arduino
//
int testArduinoConnect() {

    return 0;

} // END testArduincConnect

// MAIN
int main() {

    testSimConnect();

    try {
        hSerial = CreateFile("COM8", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);

        if (hSerial == INVALID_HANDLE_VALUE) {
            logMessage(logOutputTypeSet, LOG_ERROR, "main(): Error opening serial port!");
            return 0;
        }

        DCB dcbSerialParams = { 0 };
        dcbSerialParams.DCBlength = sizeof(dcbSerialParams);

        if (!GetCommState(hSerial, &dcbSerialParams)) {
            logMessage(logOutputTypeSet, LOG_ERROR, "main(): Error getting the state of the serial port!");
            return 1;
        }

        dcbSerialParams.BaudRate = CBR_115200;
        dcbSerialParams.ByteSize = 8;
        dcbSerialParams.StopBits = ONESTOPBIT;
        dcbSerialParams.Parity = NOPARITY;

        if (!SetCommState(hSerial, &dcbSerialParams)) {
            logMessage(logOutputTypeSet, LOG_ERROR, "main(): Error setting the state of the port!");
            return 1;
        }

        // Start the receive thread
        thread receiverThread(receiveData);

        /*
        // Send data to Arduino every 2 seconds
        while (true) {
            string message = "Hello from C++!";
            sendData(message);
            cout << "Sent: " << message << endl;

            // Wait for 2 seconds before sending the next message
            this_thread::sleep_for(chrono::seconds(2));
        }
        */

        receiverThread.join();

        // Close the serial port when done
        CloseHandle(hSerial);

    } catch (const exception &e) {
        logMessage(logOutputTypeSet, LOG_ERROR, "main(): Exception thrown: ");
        cerr << "Exception: " << e.what() << endl;
    }

    return 0;

} //END main