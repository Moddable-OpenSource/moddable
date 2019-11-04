#!/usr/bin/python
#
# Invoked when mcconfig is invoked with -n (do not flash)
# Records to-be flashed files and offset
# This is written without separators or any formatting, so it is easy
# to use as arguments for esptool.py with something like $(cat map.txt)

import os
import sys

print( "Skip flashing device: esptool.py arguments will be in " + os.getcwd() + os.path.sep + "map.txt" )

out = open("map.txt","w") 
 
# argv[0] is name of script
for i in range(1, len(sys.argv), 2):
    addr    = sys.argv[i]
    binfile = sys.argv[i+1]
    out.write( addr + " " + binfile + "\n" )

out.close()
