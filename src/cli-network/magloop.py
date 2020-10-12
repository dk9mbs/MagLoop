#!/usr/bin/python3.7
import argparse
import tkinter as tk

from maglooplib import MagLoopLib
from maglooplib import MagLoopDirection, MagLoopStepSize

parser = argparse.ArgumentParser(description='Get iq datastream form radio device')

parser.add_argument("-u", "--url", dest="url",
                         help="url http://magloop", metavar="URL", default='http://magloop')
parser.add_argument("-c", "--command", dest="command",
                         help="Command", metavar="COMMAND")
parser.add_argument("-s", "--steps", dest="steps",
                         help="Steps", metavar="STEPS")
parser.add_argument("-p", "--position", dest="position",
                         help="move to an absolute position", metavar="POSITION")
parser.add_argument("-q", "--quiet",
                         action="store_false", dest="verbose", default=True,
                         help="don't print status messages to stdout")

args=parser.parse_args()
command=args.command
steps=args.steps
position=args.position

magloop=MagLoopLib(args.url)
if command=='move_absolute':
    result=magloop.move_absolute(position)
elif command=='move_relative':
    result=magloop.move_relative(MagLoopDirection.INCREASE, MagLoopStepSize.BIG)
elif command=='position':
    result=magloop.current_position()
else:
    print("ERROR")

print(result)
print(magloop.current_position())

