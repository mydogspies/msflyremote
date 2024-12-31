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

// ARDUINO setup
// Mutex for synchronizing the serial communication
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

    // VERSION 1 - receive buffer

    /*
    cout<<"Receiving data from Arduino"<<endl;

    try {
        char data[6];  // Buffer to hold incoming data
        DWORD bytesRead;

        while (true) {
            if (ReadFile(hSerial, data, sizeof(data) - 1, &bytesRead, NULL)) {
                if (bytesRead > 0) {
                    data[bytesRead] = '\0';  // Null-terminate the received data

                    // Lock mutex to safely print from the main thread
                    mtx.lock();
                    cout << "Received: " << data << endl;
                    mtx.unlock();
                }
            } else {
                cerr << "Error reading from serial port." << endl;
            }
        }
    } catch (...) {
        cerr << "Error reading from serial port. Error code: " << GetLastError() << endl;
    }
    */

    // VERSION 2 - receive stream

    cout << "Receiving data from Arduino" << endl;

    try {
        vector<char> byteStream;  // Vector to store incoming byte stream
        DWORD bytesRead;
        char byte;  // Temporary variable to hold each byte

        while (true) {
            // Read data byte-by-byte from the serial port
            if (ReadFile(hSerial, &byte, sizeof(byte), &bytesRead, NULL)) {
                if (bytesRead > 0) {
                    byteStream.push_back(byte);  // Add byte to the byte stream

                    // Optional: Check for a delimiter or end of data (e.g., newline)
                    if (byte == '\n') {  // Assuming data ends with newline
                        // Convert the byte stream to a string
                        std::string receivedData(byteStream.begin(), byteStream.end()-1);
                        // Lock mutex to safely print from the main thread
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
        // Lock mutex before sending data
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
        // Open the serial port (replace "COM3" with the correct port on your system)
        hSerial = CreateFile("COM8", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);

        if (hSerial == INVALID_HANDLE_VALUE) {
            cerr << "Error opening serial port! Error code: " << GetLastError() << endl;
            return 1;
        }

        DCB dcbSerialParams = { 0 };
        dcbSerialParams.DCBlength = sizeof(dcbSerialParams);

        // Get the current serial port parameters
        if (!GetCommState(hSerial, &dcbSerialParams)) {
            cerr << "Error getting serial port state!" << endl;
            return 1;
        }

        // Set the baud rate, parity, etc.
        dcbSerialParams.BaudRate = CBR_115200;
        dcbSerialParams.ByteSize = 8;
        dcbSerialParams.StopBits = ONESTOPBIT;
        dcbSerialParams.Parity = NOPARITY;

        // Set the updated parameters
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

        receiverThread.join();  // Ensure the receiver thread finishes before exiting

        // Close the serial port when done
        CloseHandle(hSerial);

    } catch (const exception &e) {
        cerr << "Exception: " << e.what() << endl;
    }

    return 0;

} //END main