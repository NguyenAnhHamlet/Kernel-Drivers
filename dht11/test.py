import time

def read_dht11():
    device_path = "/dev/dht11_driver"
    
    try:
        with open(device_path, "r") as device:
            data = device.read().strip()
            print(f"DHT11 Sensor Data: {data}")
    except FileNotFoundError:
        print(f"Error: Device {device_path} not found. Make sure the driver is loaded.")
    except PermissionError:
        print(f"Error: Permission denied. Try running as root.")
    except Exception as e:
        print(f"Error reading from {device_path}: {e}")

if __name__ == "__main__":
    while True:
        read_dht11()
        time.sleep(2)  # Read every 2 seconds

