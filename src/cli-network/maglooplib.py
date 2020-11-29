#!/usr/bin/python3.7
import requests
import json
import sys
import time
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
        self._current_position=0
        self.__read_current_position()

    def get_current_position(self):
        return self._current_position

    """
    read the mapping file
    mappings: list with mapping informations
    frequency: f in MHz
    """
    def move_by_frequency(self, mappings, frequency):
        f_last=0
        step_last=0
        for mapping in mappings:
            f=float(mapping['f'])
            step=int(mapping['step'])

            if f<=frequency:
                diff_last=f_last-frequency
                diff_current=frequency-f
                diff=diff_current-diff_last
                if diff>0:
                    f=f_last
                    step=step_last
                break

            f_last=f
            step_last=step

        self.move_absolute(step, True)
        return step

    """
    Move the capacitor to the target position in steps
    """
    def move_absolute(self, target_position, wait=False):
        url=f"{self._url}/api?command=MOVE&steps={target_position}"
        r=requests.get(url)
        if r.status_code!=200:
            raise NameError(f"{r.status_code} {r.text}")

        current_position=-1
        if wait:
            self._wait_for_position(target_position)

        return {"result": r.text, "status_code": r.status_code}

    """
    Move the stepper relative to the current position.
    you can choose between a small and a big step.
    """
    def move_relative(self,direction=MagLoopDirection.INCREASE, step_size=MagLoopStepSize.SMALL, wait=False):
        self.__read_current_position()
        position=0
        if step_size==MagLoopStepSize.SMALL and direction==MagLoopDirection.INCREASE:
            position=self._current_position+10
        elif step_size==MagLoopStepSize.BIG and direction==MagLoopDirection.INCREASE:
            position=self._current_position+100
        elif step_size==MagLoopStepSize.SMALL and direction==MagLoopDirection.DECREASE:
            position=self._current_position-10
        elif step_size==MagLoopStepSize.BIG and direction==MagLoopDirection.DECREASE:
            position=self._current_position-100
        return self.move_absolute(position, wait)

    def _wait_for_position(self, target_position):
        current_position=-1
        while current_position!=int(target_position):
            self.__read_current_position()
            current_position=int(self._current_position)
            print(".", end = '', flush=True)
            time.sleep(1.0)
        print("")

    """
    Get the current position from the capacitor in steps
    """
    def __read_current_position(self):
        url=f"{self._url}/api?command=GETCURRENTPOS"
        r=requests.get(url)
        if r.status_code!=200:
            raise NameError(f"{r.status_code} {r.text}")

        self._current_position=int(r.text)




if __name__=='__main__':
    magloop=MagLoopLib('http://magloop')
    #print(magloop.move_relative(MagLoopDirection.INCREASE, MagLoopStepSize.BIG))

    import json
    f=open('map.json')
    map=json.loads(f.read())
    #print(magloop.move_by_frequency(map,7.078))
    #print(magloop.move_by_frequency(map,14.074))
    print(magloop.move_by_frequency(map,28.074))
    #magloop.move_absolute(0, True)
    #magloop.move_absolute(8500, True)
    #magloop.move_absolute(0, True)


