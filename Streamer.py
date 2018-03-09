import serial
import sliplib
import struct
import sys
from time import sleep

try:
    inputFile = open('jedi.raw', 'rb')
except:
    print('Error opening File')

try:
    ser = serial.Serial('COM13', baudrate=230400, bytesize=serial.EIGHTBITS,
                        parity=serial.PARITY_NONE, stopbits=serial.STOPBITS_ONE)
except:
    print('Error opening serial port')

driver = sliplib.Driver()
# while True:
#     on = 1
#     off = 0
#     s = struct.Struct('b')
#     packet = driver.send(s.pack(on))
#     ser.write(packet)
#     sleep(.5)
#     s = struct.Struct('b')
#     packet = driver.send(s.pack(off))
#     ser.write(packet)
#     sleep(.5)

done = False
while not done:
    if ser.in_waiting:
        if ser.read(1) == b'W':
            print('Waiting')
            sleep(.007)
    try:
        data = bytearray(inputFile.read(128))
    except:
        done = True
    ser.write(data)

inputFile.close()
