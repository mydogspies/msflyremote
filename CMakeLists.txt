cmake_minimum_required(VERSION 3.29)
project(msflyremotec)

set(CMAKE_CXX_STANDARD 20)
set(SOURCES main.cpp "C:/MSFS SDK/SimConnect SDK/include/SimConnect.h" "C:/Users/silenceisgrand/AppData/Local/Packages/Microsoft.FlightSimulator_8wekyb3d8bbwe/LocalCache/Packages/Community/pmdg-aircraft-736/Documentation/SDK/PMDG_NG3_SDK.h")

add_executable(msflyremotecpp ${SOURCES})

link_directories("C:/MSFS SDK/SimConnect SDK/lib")

target_include_directories(msflyremotecpp PRIVATE "C:/MSFS SDK/SimConnect SDK/include" "C:/Users/silenceisgrand/AppData/Local/Packages/Microsoft.FlightSimulator_8wekyb3d8bbwe/LocalCache/Packages/Community/pmdg-aircraft-736/Documentation/SDK")
target_link_libraries(msflyremotecpp "C:/MSFS SDK/SimConnect SDK/lib/SimConnect.dll" "C:/MSFS SDK/SimConnect SDK/lib/SimConnect.lib")