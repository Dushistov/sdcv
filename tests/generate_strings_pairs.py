#!/usr/bin/env python3

import random, sys

fname = "/home/evgeniy/projects/competitions/words/data/words.txt"

with open(fname, "r") as fin:
    words = sorted(set([word.strip() for word in fin.readlines()]))
    res = []
    for i in range(0, len(words)):
        max_idx = len(words) - 1
        idx1 = random.randint(0, max_idx)
        idx2 = random.randint(0, max_idx)
        res.append((words[idx1], words[idx2]))

    letters = "abcdefghijklmnopqrstuvwxyzабвгдеёжзийклмнопрстуфкцчщьъэюя"
    letters += letters.upper()
    letters  += " \t!@#$%^&*()[]"
    
    def gen_word(req_word_len):
        max_idx = len(letters) - 1
        res = ""
        for i in range(0, req_word_len):
            res += letters[random.randint(0, max_idx)]
        return res

    for i in range(0, 10000):
        l1 = random.randint(1, 100)
        l2 = random.randint(1, 100)
        res.append((gen_word(l1), gen_word(l2)))

    for i in range(0, 10000):
        l1 = random.randint(1, 100)
        res.append((gen_word(l1), gen_word(l1)))

    for item in res:
        print(item[0])
        print(item[1])
        

    
        
