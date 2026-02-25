# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

# Enable console on USB
if(NOT cdc-acm-console IN_LIST SNIPPET)
  set(SNIPPET cdc-acm-console ${SNIPPET} CACHE STRING "" FORCE)
endif()

set(BOARD teensy41)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(eurorack-dsp)

target_sources(app PRIVATE
  src/main.c
  src/analog_in.c
  src/audio.c
  src/dsp.c
  src/leds.c
)
