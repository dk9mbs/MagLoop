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
        line=readLine(port, timeout)
        if line==None and timeout>0:
            timeoutCounter+=1
            print('timeout %s' % timeoutCounter, file=sys.stderr)
            if timeoutCounter>timeout:
                return None
        else:
            if line.startswith(text):
                return line

# Class rig
#
#
import socket
import logging

logger=logging.getLogger(__name__)

class RigCtl:
    _sock=None
    _cfg=None

    def __init__(self,cfg, **args):
        self._cfg=cfg
        try:
            self._sock=socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self._sock.settimeout(1)
            self._sock.connect((self._cfg['host'], self._cfg['port']))
            self._sock.settimeout(5)
        except Exception as e:
            logger.exception(e)
            raise
        pass

    def __del__(self):
        self._sock.close()
        pass

    def get_rig(self, parameter):
        parameter=str(parameter).lower()
        if not str(parameter).startswith('+'):
            parameter='+%s' % parameter

        self._sock.send(bytes('%s\n' % (parameter) , 'UTF-8'  ))
        response=self._sock.recv(512).decode('UTF-8')
        tmp=response.split('\n')

        result=dict()
        result_item=dict()
        for item in tmp:
            if item.startswith('RPRT'):
                result['result']=str(item.replace('RPRT','')).strip()
            elif str(item.strip()).endswith(':'):
                result['command']=item.replace(':','')
            else:
                keyvalue=item.split(':')
                if len(keyvalue) == 2:
                    key=keyvalue[0].strip()
                    value=keyvalue[1].strip()
                    if key!='':
                        result_item[key]=value   
        
        result['response']=result_item
        return result


initSerial(port)
ser = serial.Serial(port = port, baudrate = baudrate, timeout=None)
ser.isOpen()

print ('Command => %s' % command, file=sys.stderr)
ser.write(bytes('%s' % command, 'utf-8'))
ser.write(b'\r')
rcv = waitForLine(ser, "OK", timeout=0)
print ('%s' % rcv)


#rig=RigCtl({'host':'localhost', 'port': 4532})
#print(rig.get_rig("f"))

ser.close()
