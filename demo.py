#!/usr/bin/python3
#-*- encoding: utf-8 -*-

from cnn import *

def demo1():
	run("img/dice.jpg", [None, 1], [AVG, EDGE], anim = True)

def demo2():
	run("img/cross.jpg", None, [HL3], anim = True)

msg = """
Choose a demo, or 0 to exit:
  (1) Outlines
  (2) Halftoning
"""


demo = int(input(msg))
while demo != 0:
	if demo == 1:
		demo1()
	elif demo == 2:
		demo2()
	demo = int(input(msg))

