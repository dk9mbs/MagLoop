#!/usr/bin/python3.7
import argparse
import tkinter as tk
import json

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


class Gui:
    def increase_small(self):
        result=magloop.move_relative(MagLoopDirection.INCREASE, MagLoopStepSize.SMALL,True)
        self.text.set(magloop.get_current_position())

    def decrease_small(self):
        result=magloop.move_relative(MagLoopDirection.DECREASE, MagLoopStepSize.SMALL,True)
        self.text.set(magloop.get_current_position())

    def increase_big(self):
        result=magloop.move_relative(MagLoopDirection.INCREASE, MagLoopStepSize.BIG,True)
        self.text.set(magloop.get_current_position())

    def decrease_big(self):
        result=magloop.move_relative(MagLoopDirection.DECREASE, MagLoopStepSize.BIG,True)
        self.text.set(magloop.get_current_position())

    def max_c(self):
        result=magloop.move_absolute(0,True)
        self.text.set(magloop.get_current_position())

    def min_c(self):
        result=magloop.move_absolute(8500,True)
        self.text.set(magloop.get_current_position())

    def set_qrg(self, event):
        freq=float(event.widget.get())
        f=open('map.json')
        map=json.loads(f.read())
        f.close()
        result=magloop.move_by_frequency(map, freq)
        self.text.set(magloop.get_current_position())

    def __init__(self):
        root=tk.Tk()
        root.title("MagLoop by AG5ZL/DK9MBS")
        self.text=tk.StringVar()
        self.text.set(magloop.get_current_position())

        tk.Label(root, textvariable=self.text).pack(side=tk.LEFT)

        tk.Button(root, text="<", command=self.increase_small).pack(side=tk.LEFT)
        tk.Button(root, text="<<", command=self.increase_big).pack(side=tk.LEFT)
        tk.Button(root, text=">>", command=self.decrease_big).pack(side=tk.LEFT)
        tk.Button(root, text=">", command=self.decrease_small).pack(side=tk.LEFT)
        tk.Button(root, text="min. c", command=self.max_c).pack(side=tk.LEFT)
        tk.Button(root, text="max. c", command=self.min_c).pack(side=tk.LEFT)

        entry=tk.Entry(root, text="enter qrg")
        entry.bind("<Return>", self.set_qrg)
        entry.pack()

        root.mainloop()


magloop=MagLoopLib(args.url)
if command=='move_absolute':
    result=magloop.move_absolute(position,True)
elif command=='move_increase':
    result=magloop.move_relative(MagLoopDirection.INCREASE, MagLoopStepSize.SMALL,True)
elif command=='move_decrease':
    result=magloop.move_relative(MagLoopDirection.DECREASE, MagLoopStepSize.SMALL,True)
elif command=='position':
    result=magloop.get_current_position()
elif command=='gui':
    gui=Gui()
else:
    print("ERROR")

print(result)
print(magloop.get_current_position())
