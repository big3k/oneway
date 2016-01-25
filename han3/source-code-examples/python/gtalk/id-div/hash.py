#!/usr/local/bin/python
# -*- coding: koi8-r -*-
# $Id: bot.py,v 1.2 2006/10/06 12:30:42 normanr Exp $
import sys, time
import random

ipfile = open('msnip.txt', 'r')
ipmsg = ipfile.read()
ipfile.close()

iplist = ipmsg.splitlines()

print iplist[0]

nips = len(iplist)
random.seed("user")

mymsg=''

for id in range(0, 6):
      mymsg = mymsg + "\n" + iplist[random.randint(0, nips-1)]

print mymsg 

