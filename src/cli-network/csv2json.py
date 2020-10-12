#!/usr/bin/python3.7
import sys
import json

def convert_csv_file (filename, head_line=True):
    f=open (filename,'r')
    lines=f.readlines()
    line_number=0
    mapping=[]
    for line in lines:
        if line_number!=0 or not head_line:
            step=int(line.split(';')[0])
            frequency=float(line.split(';')[1])
            sys.stderr.write(f".")
            mapping.append({"step":step, "f": frequency/1000000, "unit":"MHz", "line": line_number})

        line_number+=1

    sys.stderr.write(f"\n")
    return mapping

input_file=sys.argv[1]

sys.stderr.write(f"reading import csv file {input_file} ...\n")
mapping=convert_csv_file(input_file)
print(json.dumps(mapping))
sys.stderr.write(f"ready!\n")
