// msflyremote
// version 0.1
// interfaces Arduino/Nextion with MSFS 2020 via this script. This repo is just a proof of concept and not
// a full working library. No support and no guarantee it works on your setup.
// requires the MSFS2020 SDK and the PMDG SDK

#include <windows.h>
#include <iostream>
#include <vector>
using namespace std;
#include <string>
#include<stdlib.h>
#include "SimConnect.h"
#include <thread>
#include <mutex>

// MSFS 2020 setup
HANDLE  hSimConnect = NULL;

// Threading setup
std::mutex mtx;
// Global serial port handle
HANDLE hSerial;


//
// MSFS 2020 functions
//

// Test connection to msfs2020
//
void testSimConnect() {

    HRESULT hr;

    if (SUCCEEDED(SimConnect_Open(&hSimConnect, "Open and Close", nullptr, 0, 0, 0))) {

        printf("\nConnected to Flight Simulator!\n");
        hr = SimConnect_Close(hSimConnect);
        printf("\nDisconnected from Flight Simulator\n");

    } else
        printf("\nFailed to connect to Flight Simulator\n");
} // END testSimConnnect


//
// ARDUINO functions
//

// Function to receive data from Arduino
void receiveData() {

    cout << "Receiving data from Arduino" << endl;

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
                cerr << "Error reading from serial port." << endl;
            }
        }

    } catch (...) {
        cerr << "Error reading from serial port. Error code: " << GetLastError() << endl;
    }


}

// Function to send data to Arduino
void sendData(const string& message) {
    try {
        mtx.lock();
        DWORD bytesWritten;
        if (!WriteFile(hSerial, message.c_str(), message.length(), &bytesWritten, NULL)) {
            cerr << "Error writing to serial port." << endl;
        }
        mtx.unlock();
    } catch (...) {
        cerr << "An error occurred while sending data." << endl;
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
            cerr << "Error opening serial port! Error code: " << GetLastError() << endl;
            return 1;
        }

        DCB dcbSerialParams = { 0 };
        dcbSerialParams.DCBlength = sizeof(dcbSerialParams);

        if (!GetCommState(hSerial, &dcbSerialParams)) {
            cerr << "Error getting serial port state!" << endl;
            return 1;
        }

        dcbSerialParams.BaudRate = CBR_115200;
        dcbSerialParams.ByteSize = 8;
        dcbSerialParams.StopBits = ONESTOPBIT;
        dcbSerialParams.Parity = NOPARITY;

        if (!SetCommState(hSerial, &dcbSerialParams)) {
            cerr << "Error setting serial port state!" << endl;
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
        cerr << "Exception: " << e.what() << endl;
    }

    return 0;

} //END main