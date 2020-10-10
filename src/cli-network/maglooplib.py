#!/usr/bin/python3.7

import requests
import json
from enum import Enum

class MagLoopStepSize(Enum):
    SMALL=1
    BIG=2

class MagLoopDirection(Enum):
    INCREASE=1
    DECREASE=2

class MagLoopLib:
    def __init__(self, url):
        self._url=url

    """
    Move the capacitor to the target position in steps
    """
    def move_absolute(self, target):
        url=f"{self._url}/api?command=MOVE&steps={target}"
        r=requests.get(url)
        if r.status_code!=200:
            raise NameError(f"{r.status_code} {r.text}")

        return {"result": r.text, "status_code": r.status_code}

    """
    Get the current position from the capacitor in steps
    """
    def current_position(self):
        url=f"{self._url}/api?command=GETCURRENTPOS"
        r=requests.get(url)
        if r.status_code!=200:
            raise NameError(f"{r.status_code} {r.text}")

        return {"result": r.text, "status_code": r.status_code}


    def move_relative(self,direction=MagLoopDirection.INCREASE, step_size=MagLoopStepSize.SMALL):
        if step_size==MagLoopStepSize.SMALL and direction==MagLoopDirection.INCREASE:
            command="INCREASE_SMALL_STEP"
        elif step_size==MagLoopStepSize.BIG and direction==MagLoopDirection.INCREASE:
            command="INCREASE_BIG_STEP"
        elif step_size==MagLoopStepSize.SMALL and direction==MagLoopDirection.DECREASE:
            command="DECREASE_SMALL_STEP"
        elif step_size==MagLoopStepSize.BIG and direction==MagLoopDirection.DECREASE:
            command="DECREASE_BIG_STEP"

        url=f"{self._url}/api?command={command}"
        r=requests.get(url)
        if r.status_code!=200:
            raise NameError(f"{r.status_code} {r.text}")

        return {"result": r.text, "status_code": r.status_code}



if __name__=='__main__':
    move=True

    magloop=MagLoopLib('http://192.168.2.114')

    if move==True:
        print(magloop.move_absolute(0))
        print(magloop.move_absolute(8500))
        print(magloop.move_absolute(4200))
        print(magloop.move_absolute(7200))
        print(magloop.move_absolute(10))
        print(magloop.move_absolute(8000))
        print(magloop.move_absolute(8500))
        print(magloop.move_absolute(200))
        print(magloop.move_absolute(350))
        print(magloop.move_absolute(7350))
        print(magloop.move_absolute(8500))

    #print(magloop.move_relative(MagLoopDirection.INCREASE, MagLoopStepSize.BIG))
    print(magloop.current_position())

