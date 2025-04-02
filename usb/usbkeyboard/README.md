USB Keyboard Device Driver

1. Overview
A simple Linux kernel driver for handling USB keyboard input. This driver captures and processes keypress events, interfacing with the USB subsystem to provide basic keyboard functionality.

2. Features

Detects and reads keyboard keypress events.
Interfaces with the USB HID (Human Interface Device) subsystem.
Loadable and unloadable as a kernel module.

4. Installation
   
Building and Loading the Module
Run the following commands to compile and load the driver:
make all       # Compile the module  
make load      # Load the module into the kernel  

5. Unloading the Module
To remove the module from the kernel, use:
make unload 
 
6. Dependencies
Linux kernel headers
USB core and HID subsystem support

7. Usage
Once loaded, the driver will capture keypress events. You can check logs using:
make log
