cmake_minimum_required(VERSION 3.16)
project(DisplayBackendDiagnosticTool)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(QT NAMES Qt6 REQUIRED COMPONENTS Widgets Gui)
find_package(Qt6 REQUIRED COMPONENTS Widgets Gui)

set(SOURCES
    src/main.cpp
    src/gpu_tray.cpp
    src/tray_popup.cpp
    src/resources.qrc
)


set(HEADERS
    src/gpu_tray.h
    src/tray_popup.h
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(gpu_switcher_tray ${SOURCES} ${HEADERS})

target_link_libraries(gpu_switcher_tray Qt6::Widgets Qt6::Gui)