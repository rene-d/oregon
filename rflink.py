#! /usr/bin/env python3
# rene-2 02/2019

# stats of RFLink output
# miniterm.py /dev/ttyACM0 57600 --raw | tee -a rflink.log

import re

c = dict()
for i in open("rflink.log"):
    v = i.strip().split(';')[2:]

    if len(v) == 2:
        continue

    if not all(j == '' or re.match(r'\w+=.+', j) for j in v[1:]):
        continue

    key = v[0]
    if key not in c:
        c[key] = set()

    c[key].add(",".join(v[1:-1]))

for k, v in c.items():
    print(k)
    for i in sorted(v):
        print("  ", i)
