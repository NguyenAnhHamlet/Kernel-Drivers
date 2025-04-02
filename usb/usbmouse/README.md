---- USB mouse device driver ---- 

1. Overview
A simple Linux kernel driver for handling USB mouse input. This driver captures and processes mouse movement events, providing basic functionality for interacting with the system.

2. Features
- Detects and reads mouse movement events.
- Interfaces with the USB subsystem to handle input data.
- Loadable and unloadable as a kernel module.

3. Installation
Building and Loading the Module
Run the following commands to compile and load the driver:
make all       # Compile the module  
make load      # Load the module into the kernel

To remove the module from the kernel, use:
make unload

Dependencies
Linux kernel headers
USB core and HID subsystem support

Usage: 
Once loaded, the driver will listen for mouse movement events. You can check logs using:
make log
