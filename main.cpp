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

using namespace std;
#include "SimConnect.h"
#include "Config.h"

HANDLE hSimConnect = nullptr;
HANDLE hSerial = nullptr;

std::mutex mtx;
Config globalConfig;
LogLevel logLevelSet;
LogOutput logOutputTypeSet;

//
// LOGGER DEFINITIONS
//

//
// FUNCTIONS
//

// constexpr std::array<const char *, LogLevel::LOG_LEVELS> levelStrings = {
//     {
//         "LOG_DEBUG",
//         "LOG_INFO",
//         "LOG_WARNING",
//         "LOG_ERROR"
//     }
// };

// const char *levelToString(const LogLevel level) {
//     if (level < LogLevel::LOG_LEVELS > 0) {
//         return levelStrings[level];
//     }
//     return "Unknown";
// }

// get current time
//
char *getTime() {
    time_t rawtime = time(nullptr);
    struct tm *timeinfo = localtime(&rawtime);
    char *date_time = asctime(timeinfo);
    return date_time;
} // END getTime

// Logging function
// with option to log to terminal or to file
//
void logMessage(LogOutput outputType, LogLevel level, const std::string &message,
                const std::string &filename = "log.txt") {
    std::ostream *outputStream = nullptr;
    std::string msg;

    // Determine the output stream based on the output type
    if (outputType == LogOutput::LOG_TO_FILE && logLevelSet <= level) {
        std::ofstream logFile(filename, std::ios::out | std::ios::app);

        if (!logFile.is_open() || !logFile.good()) {
            std::cerr << "Failed to open log file: " << filename << std::endl;
            return;
        }
        outputStream = &logFile;

        switch (level) {
            case LogLevel::LOG_DEBUG:
                (*outputStream) << getTime() << " | DEBUG: ";
                break;
            case LogLevel::LOG_INFO:
                (*outputStream) << getTime() << " | INFO: ";
                break;
            case LogLevel::LOG_WARNING:
                (*outputStream) << getTime() << " | WARNING: ";
                break;
            case LogLevel::LOG_ERROR:
                (*outputStream) << getTime() << " | ERROR: ";
                break;
            default:
                (*outputStream) << getTime() << " | UNKNOWN: ";
                break;
        }
        (*outputStream) << message << std::endl;
    } else {
        if (logLevelSet <= level) {
            cout << getTime() << " : " << message << endl;
        }
    }
} // END logMessage

//
// MSFS 2020 functions
//

// Test connection to msfs2020
//
void testSimConnect() {
    if (SUCCEEDED(SimConnect_Open(&hSimConnect, "Open and Close", nullptr, 0, nullptr, 0))) {
        logMessage(logOutputTypeSet, LogLevel::LOG_INFO, "testSimConnect(): Connected to MSFS 2020.");
        HRESULT hr = SimConnect_Close(hSimConnect);
        logMessage(logOutputTypeSet, LogLevel::LOG_WARNING, "testSimConnect(): Disconnected to MSFS 2020!");
    } else
        logMessage(logOutputTypeSet, LogLevel::LOG_ERROR, "testSimConnect(): Failed to connect to MSFS 2020");
} // END testSimConnect


//
// ARDUINO functions
//

// Function to receive data from Arduino
void receiveData() {
    logMessage(logOutputTypeSet, LogLevel::LOG_INFO, "receiveData(): Listening to Arduino.");

    try {
        vector<char> byteStream;
        DWORD bytesRead;
        char byte;

        while (true) { // REFACTOR
            if (ReadFile(hSerial, &byte, sizeof(byte), &bytesRead, nullptr)) {
                if (bytesRead > 0) {
                    byteStream.push_back(byte);

                    if (byte == '\n') {
                        // Assuming data ends with newline
                        std::string receivedData(byteStream.begin(), byteStream.end() - 1);
                        mtx.lock();
                        logMessage(logOutputTypeSet, LogLevel::LOG_DEBUG, "receiveData() - Received from serial: " + receivedData);
                        cout << "Received: " << receivedData << endl;
                        mtx.unlock();
                        byteStream.clear();
                    }
                }
            } else {
                logMessage(logOutputTypeSet, LogLevel::LOG_ERROR, "receiveData(): Error reading from open serial port.");
            }
        }
    } catch (...) {
        logMessage(logOutputTypeSet, LogLevel::LOG_ERROR, "receiveData(): Error while trying to read serial port.");
    }
}

// Function to send data to Arduino
void sendData(const string &message) {
    try {
        mtx.lock();
        DWORD bytesWritten;
        if (!WriteFile(hSerial, message.c_str(), message.length(), &bytesWritten, nullptr)) {
            logMessage(logOutputTypeSet, LogLevel::LOG_ERROR, "sendData(): Error writing data to open port.");
        }
        mtx.unlock();
    } catch (...) {
        logMessage(logOutputTypeSet, LogLevel::LOG_ERROR, "sendData(): Error while trying to send data.");
    }
}

// MAIN
int main() {

    // Get settings from config
    if (Config config; config.loadConfig("config.txt")) {
        logLevelSet = config.getLogLevel("log_level", LogLevel::LOG_ERROR);
        logOutputTypeSet = config.getLogTarget("log_target", LogOutput::LOG_TO_TERMINAL);
    } else {
        std::cerr << "ERROR!! Failed to load configuration\n";
    }

    // simconnect
    testSimConnect();

    try {
        hSerial = CreateFile("COM8", GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);

        if (hSerial == INVALID_HANDLE_VALUE) {
            logMessage(logOutputTypeSet, LogLevel::LOG_ERROR, "main(): Error opening serial port!");
            return 0;
        }

        DCB dcbSerialParams = {0};
        dcbSerialParams.DCBlength = sizeof(dcbSerialParams);

        if (!GetCommState(hSerial, &dcbSerialParams)) {
            logMessage(logOutputTypeSet, LogLevel::LOG_ERROR, "main(): Error getting the state of the serial port!");
            return 0;
        }

        dcbSerialParams.BaudRate = CBR_115200;
        dcbSerialParams.ByteSize = 8;
        dcbSerialParams.StopBits = ONESTOPBIT;
        dcbSerialParams.Parity = NOPARITY;

        if (!SetCommState(hSerial, &dcbSerialParams)) {
            logMessage(logOutputTypeSet, LogLevel::LOG_ERROR, "main(): Error setting the state of the port!");
            return 0;
        }

        // Start the receive thread
        thread receiverThread(receiveData);
        receiverThread.join();

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

        // Close the serial port when done
        CloseHandle(hSerial);
    } catch (const exception &e) {
        logMessage(logOutputTypeSet, LogLevel::LOG_ERROR, "main(): Exception thrown while closing handle.");
        cerr << "Exception: " << e.what() << endl;
        return 0;
    }

    return 1;
} //END main
