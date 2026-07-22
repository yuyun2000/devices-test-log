cmake_minimum_required(VERSION 3.16)

if(NOT DEFINED ENV{ESP_BSP_PATH})
    message(FATAL_ERROR "Set ESP_BSP_PATH to an esp-bsp checkout containing bsp/m5stack_stopwatch")
endif()

set(EXTRA_COMPONENT_DIRS "$ENV{ESP_BSP_PATH}/bsp/m5stack_stopwatch")
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(stopwatch_bsp_hil)
