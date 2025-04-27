# This code is tweaked from the Pyduino library

import serial
import time
import serial.tools.list_ports

arduino : serial.Serial

def Init():
    global arduino
    # Find arduino port
    ports = list(serial.tools.list_ports.comports())
    #serialBytes = []
    found = False
    for p in ports:
        if "Serial" in p.description:
            # Connect to arduino
            arduino = serial.Serial(port=p.device, baudrate=115200, timeout=1)
            found = True
            print("Connected")

    if not found:
        print("No arduino found! All ports:")
        for p in ports:
            print(p)
        return -1
    return 0

Init()

# Date and time
timestr = time.strftime("%Y_%m_%d-%H_%M_%S")

with open("log_" + timestr + ".csv", "w") as f:
    while True:
        line = arduino.readline().decode().strip()
        if line:
            timestamp = int(time.time() * 1000)
            f.write(f"{timestamp},{line}\n")