import serial

s = serial.Serial("/dev/ttyACM0", 115200)

with open("serialcam.dat", "wb") as fh:
    while True:
        d = s.read(1024)
        if not d:
            break
        fh.write(d)
