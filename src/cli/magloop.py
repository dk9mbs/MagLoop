#!/usr/bin/python3.7

from __future__ import print_function
import serial
import time
import argparse
import os
import subprocess
import sys


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

port = "/dev/ttyUSB0"
baudrate = 9600
command="SP"

if args.device != None:
    port = args.device

if args.baudrate != None:
    baudrate = args.baudrate

if args.command != None:
    command=args.command

print('Port: %s' % port, file=sys.stderr)
print('Baud: %s' % baudrate, file=sys.stderr)
print('Command: %s' % command, file=sys.stderr)

def initSerial(port):
    #stty -F /dev/ttyUSB0 -hupcl
    #https://playground.arduino.cc/Main/DisablingAutoResetOnSerialConnection/
    process=subprocess.Popen(['/bin/stty', '-F', port, '-hupcl'])
    process.wait()

def readLine(port, timeout=0):
    s = ""
    timeoutCounter=0
    while True:
        ch = port.read(1)

        # in case of timeout return a None value
        if ch==b'' and timeout>0:
            timeoutCounter+=1
            print('timeout %s' % timeoutCounter, file=sys.stderr)
            if timeoutCounter>timeout:
                return None
        else:
            s += "".join(map(chr, ch))
            if ch == b'\r' or ch == b'\n':
                return s

def waitForLine(port, text, timeout=0):
    s=""
    found=False
    timeoutCounter=0


    while True:
        line=port.readline()
        if line==None and timeout>0:
            timeoutCounter+=1
            print('timeout %s' % timeoutCounter, file=sys.stderr)
            if timeoutCounter>timeout:
                return None
        else:
            if line.startswith(bytes(text,'utf-8')):
                return line


        #ch = port.read(1)

        # in case of timeout return a None value
        #if ch==b'' and timeout>0:
        #    timeoutCounter+=1
        #    print('timeout %s' % timeoutCounter, file=sys.stderr)
        #    if timeoutCounter>timeout:
        #        return None
        #else:
        #    s +="".join(map(chr, ch))
        #    if s==text:
        #        found=True

        #    if ch == b'\r' or ch == b'\n':
        #        if found==True:
        #            return s
        #        else:
        #            s=""


initSerial(port)
ser = serial.Serial(port = port, baudrate = baudrate, timeout=None)
ser.isOpen()

print ('Command => %s' % command, file=sys.stderr)
ser.write(bytes('%s' % command, 'utf-8'))
ser.write(b'\r')

rcv = waitForLine(ser, "OK", timeout=0)
print ('%s' % rcv)

ser.close()
