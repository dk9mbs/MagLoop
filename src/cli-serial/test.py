# Serial1.py
#stty -F /dev/ttyUSB0 -hupcl
#https://playground.arduino.cc/Main/DisablingAutoResetOnSerialConnection/

import serial
import time
import argparse
import os

import subprocess


parser = argparse.ArgumentParser(description='Get iq datastream form radio device')

parser.add_argument("-d", "--device", dest="device",
                        help="Device", metavar="DEVICE")
parser.add_argument("-c", "--command", dest="command",
                        help="Command", metavar="COMMAND")
parser.add_argument("-b", "--baud-rate", dest="baudrate",
                        help="Baudrate", metavar="BAUDRATE")
parser.add_argument("-q", "--quiet",
                        action="store_false", dest="verbose", default=True,
                        help="don't print status messages to stdout")

args=parser.parse_args()

#port = "/dev/ttyUSB1"
#baudrate = 9600
#command="SP"

port = args.device
baudrate = args.baudrate
command=args.command

def initSerial(port):
    process=subprocess.Popen(['/bin/stty', '-F', port, '-hupcl'])
    process.wait()


def readLine(port):
    s = ""
    while True:
        ch = port.read()
        s += "".join(map(chr, ch))
        if ch == b'\r':
            return s

def waitFor(port, text):
    s=""
    found=False
    while True:
        ch = port.read()
        s +="".join(map(chr, ch))
        
        if s==text:
            found=True

        if ch == b'\r' or ch == b'\n':
            if found==True:
                return s
            else:
                s=""


initSerial(port)

ser = serial.Serial(port = port, baudrate = baudrate)
ser.isOpen()

print ('Command => %s' % command)
ser.write(bytes('%s\r\n' % command, 'utf-8'))
rcv = waitFor(ser, "OK")
print ('Result => %s' % rcv)

#initSerial(port)
ser.close()

