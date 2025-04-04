# DHT11 Kernel Driver

## Overview
This Linux kernel driver facilitates communication with the DHT11 temperature and humidity sensor. It provides a character device interface for readingsensor values from user space.

## Features
Implements a character driver for reading temperature and humidity data.
Supports communication between the Linux kernel and the DHT11 sensor.
Loadable and unloadable as a kernel module.

## Installation
## Building and Loading the Module
Run the following commands to compile and load the driver:
``` sh
make all       # Compile the module  
make load      # Load the module into the kernel  
```

Unloading the Module
To remove the module from the kernel, use:
``` sh
make unload  
```

## Dependencies
Linux kernel headers

## Usage
Once the module is loaded, you can read sensor values from the character device:
``` sh
cat /dev/dht11  
```

You can also check logs for driver output:
``` sh
make log
```
