#!/usr/bin/python
###################################################################################
#6/8/07 Yudong Tian
#
# test sensor class. 

import asyncore
import socket
import string
import threading
import time


class sensor_class:
    def __init__ (self, sid, lat, lon): 
        self.sid = sid
        self.lat = lat 
        self.lon = lon

    def set_data (self, data):
        self.data = data

    def get_data (self):
	return (self.sid, self.lat, self.lon, self.data)

    def set_delay (self, delay):
        self.delay  = delay 

    def get_delay (self):
        return self.delay 


if __name__ == '__main__':
    import sys
    import urlparse

    for id in range(1, 40):
       sid = 's%d' % id
       print sid

    sensor = sensor_class("sid", 100, 200)
 
    sensor.set_data(200.0)
    sensor.set_delay(12.0) 

    print "1: %s  2: %s  3: %s  4: %f " % sensor.get_data()
    print "delay = %f " % sensor.get_delay()
    print "lat: %f  lon: %f" % (sensor.lat, sensor.lon)
