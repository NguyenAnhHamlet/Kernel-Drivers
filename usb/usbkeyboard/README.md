# USB Keyboard Device Driver

## Overview
A simple Linux kernel driver for handling USB keyboard input. This driver captures and processes keypress events, interfacing with the USB subsystem to provide basic keyboard functionality.

## Features

Detects and reads keyboard keypress events.
Interfaces with the USB HID (Human Interface Device) subsystem.
Loadable and unloadable as a kernel module.

## Installation
   
## Usage
## Building and Loading the Module
Run the following commands to compile and load the driver:
make all       # Compile the module  
make load      # Load the module into the kernel  

## Unloading the Module
To remove the module from the kernel, use:
make unload 
 
## Dependencies
Linux kernel headers
USB core and HID subsystem support

## Usage
Once loaded, the driver will capture keypress events. You can check logs using:
make log
