# /bin/bash

miniterm.py --raw --exit-char 3 /dev/ttyUSB0 115200 | tee -a dump.txt
