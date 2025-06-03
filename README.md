# Smart Traffic Control System

This repository contains the source code, documentation, and test files for the Smart Traffic Control System, an embedded project developed to manage traffic at a two-lane junction. The system dynamically adjusts traffic light timings based on congestion detected by ultrasonic sensors, ensuring efficient traffic flow. It includes a secure passkey interface for system activation.

## Project Overview

The Smart Traffic Control System uses an STM32F405RGT6 microcontroller, programmed in Embedded C with STM32CubeIDE. Key features include:
- Secure access via a 4x4 keypad with passkey authentication.
- Congestion detection using four HC-SR04 ultrasonic sensors.
- Dynamic traffic light control (Red, Yellow, Green) for two lanes.
- Real-time feedback via a 16x2 LCD and two 4-digit 7-segment displays.
- Voltage compatibility ensured by a bidirectional voltage converter (3.3V–5V).

## Hardware Requirements

- STM32F405RGT6 microcontroller board
- 4x HC-SR04 ultrasonic sensors
- 2x Traffic light signal modules (Red, Yellow, Green LEDs)
- 1x 16x2 LCD display
- 2x 4-digit 7-segment displays
- 1x 4x4 keypad
- 1x Bidirectional voltage converter (3.3V–5V)
- ST-Link debugger for flashing and debugging
