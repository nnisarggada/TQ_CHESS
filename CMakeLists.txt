cmake_minimum_required(VERSION 3.11)
PROJECT(OpenCVChessbooard)

find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})

set(CMAKE_EXPORT_COMPILE_COMMANDS ON) #used for the autocomplete YouCompletePlugin for Vundle

ADD_EXECUTABLE(chessdetection src/main.cpp src/chessdetection.h)
TARGET_LINK_LIBRARIES(chessdetection ${OpenCV_LIBS})
