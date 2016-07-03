#!/usr/bin/env python3

import sys

with open(sys.argv[1], "r") as f:
    with open(sys.argv[2], "w") as out:
        words = set([word.strip() for word in f.readlines()])
        for word in words:
            out.write(word + "\n")
            out.write(word + "\n")
