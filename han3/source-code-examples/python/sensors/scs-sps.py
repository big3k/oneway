#!/usr/bin/python
###################################################################################
# Yudong Tian  $Id: scs-sps.py,v 1.4 2007/06/14 15:50:13 yudong Exp yudong $
#
# scs-sps.py:
#  Implementation of protoype SCS and SPS services. 
#  It branches off a thread to simulate sensors (sensor collection service), 
# which report data to activeMQ's topic: LISW.SCS. Any client subscribed to 
# this topic can get the sensor data from this URL:  
#
# http://192.239.84.155:18080/activemq-web-demo/message/LISW/SCS?timeout=10000&type
# =topic
#
#  This program also subscribes topc LISW.SPS to receive any notification
# regarding sensor planning service. A notification can be sent with this URL: 
#
# wget -O tmp.out --post-data='' \\
#  http://localhost:18080/activemq-web-demo/message/LISW/SPS?type=topic\&body=abc
#
#  An AJAX-based demo to interact with this sode can be run at: 
#  http://192.239.84.155:18080/activemq-web-demo/lisw-demo.html 
###################################################################################

import asyncore
import socket
import string
import threading
import time
import httplib

total_sensors=60
delay=10 
short_delay=1
#host='127.0.0.1'  
host='192.239.84.155'  
port=18080
sensors={}
# Sensor planning service (SPS)
#req='GET /activemq-web-demo/message/LISW/SPS?timeout=10000&type=topic'
req='/activemq-web-demo/message/LISW/SPS?timeout=10000&type=topic'
cookie='Cookie: JSESSIONID=1187oko6ookat\r\nConnection: keep-alive'

# Sensor collection service (SCS): station_id|lat|lon|data
post='/activemq-web-demo/message/LISW/SCS?type=topic&body=%s|%.1f|%.1f|%.1f'

class sensor_class:
    def __init__ (self, sid, lat, lon, delay):
        self.sid = sid
        self.lat = lat
        self.lon = lon
        self.delay = delay

    def set_data (self, data):
        self.data = data

    def get_data (self):
        return (self.sid, self.lat, self.lon, self.data)

    def set_delay (self, delay):
        self.delay  = delay

    def get_delay (self):
        return self.delay


class sensor_thread(threading.Thread):

    def __init__ (self, sensor):
	self._stopevent = threading.Event()
	self._sleepperiod = 1.0
	self.sensor = sensor
	threading.Thread.__init__(self)

    def run(self):
      import random
      global delay, post
      while not self._stopevent.isSet(): 

	self.sensor.set_data( random.random()*30 + 273 )   # simulate temp in K
        print '%s reading: lat: %.1f lon: %.1f data: %.1f' % self.sensor.get_data()
        req= post % self.sensor.get_data() 
	sconn = httplib.HTTPConnection(host, port) 
	sconn.request("POST", req, "", {"Cookie" : cookie}) 
	#print 'Server response:' , msg
        sconn.close() 

        time.sleep( self.sensor.get_delay() )

    def join(self, timeout=None):
	self._stopevent.set()
	threading.Thread.join(self, timeout) 


## function to get JSESSIONID from "Set-Cookie" header
## JSESSIONID will be used for subsequent transactions with the server

def get_session():

    req='GET /activemq-web-demo/'
    sockobj = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sockobj.connect((host, port))

    sockobj.send ('%s HTTP/1.0\r\n\r\n' % req )
    msg = sockobj.recv(8092) 
    ssid = msg.split('Set-Cookie: ')[-1].split(';path=')[0]
    print 'Server response:' , ssid
    cookie=ssid
    return cookie

if __name__ == '__main__':
    import sys
    import random

    pods={}
    cookie = get_session()
    if cookie:

       ## create sensor instances and fire them up 
       for id in range(1, total_sensors+1): 
	   sid = 's%d' % id
	   # lat/lon over CONUS
	   lat = 25 + 23 * random.random()
	   lon = -125 + 55 * random.random()
	   sensors[sid]=sensor_class(sid, lat, lon, delay)
           pods[sid]=sensor_thread( sensors[sid] )
	   time.sleep(random.random()*1) # a bit delay
           pods[sid].start()

        ## SPS listener using httplib
       while 1:
           conn = httplib.HTTPConnection(host, port)
           conn.request("GET", req, "", {"Cookie" : cookie} )
           data = conn.getresponse().read()
           conn.close()
           print 'Response: %s' % data 
	   if data: 
	     (clat, clon) =map(float,  data.split("|"))
             print 'Recv: lat=%.1f lon=%.1f' % (clat, clon) 
  	     for id in range(1, total_sensors+1):
                sid = 's%d' % id
	  	lat = sensors[sid].lat
	  	lon = sensors[sid].lon
	        if ( abs(lat - clat) < 4 and abs(lon - clon) < 4 ): 
                  #print 'Setting %s delay: %d' % (sid, short_delay) 
                  sensors[sid].set_delay(short_delay)
	 	else:
                  sensors[sid].set_delay(delay)

       for id in range(1, total_sensors+1): 
	   sid = 's%d' % id
           pods[sid].join()

    else: 
	
       print 'Failed to connect to server. quit ...' 



