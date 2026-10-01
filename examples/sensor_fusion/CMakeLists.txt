cmake_minimum_required(VERSION 3.16)
project(SensorFusionSample C)

set(CMAKE_C_STANDARD 99)

include_directories(Header)

add_executable(sensor_fusion_app
    Source/sensor_fusion.c
)
