#!/usr/bin/python
###################################################################################
#6/8/07 Yudong Tian
#
#mthread-http.py:
#  It branches off a thread to simulate sensors (sensor collection service), 
#which report data to activeMQ's topic: LISW.SCS. Any client subscribed to 
#this topic can get the sensor data from this URL:  
#
# http://192.239.84.155:18080/activemq-web-demo/message/LISW/SCS?timeout=10000&type=topic
#
#  This program also subscribes topc LISW.SPS to receive any notification
#regarding sensor planning service. A notification can be sent with this URL: 
#
# wget -O tmp.out --post-data='' \\
#  http://localhost:18080/activemq-web-demo/message/LISW/SPS?type=topic\&body=abc
#
###################################################################################

import asyncore
import socket
import string
import threading
import time

total_sensors=40
delay=10
host='127.0.0.1' 
port=18080
path='/'
sensors={}
# Sensor planning service (SPS)
req='GET /activemq-web-demo/message/LISW/SPS?timeout=10000&type=topic'
cookie='Cookie: JSESSIONID=1187oko6ookat\r\nConnection: keep-alive'

# Sensor collection service (SCS): station_id|lat|lon|data
post='POST /activemq-web-demo/message/LISW/SCS?type=topic&body=%s|%.1f|%.1f|%.1f'


class http_client (asyncore.dispatcher):

    def __init__ (self, host, path):
        asyncore.dispatcher.__init__ (self)
        self.path = path
        self.create_socket (socket.AF_INET, socket.SOCK_STREAM)
        self.connect ((host, port))

    def handle_connect (self):
        self.send ('%s HTTP/1.0\r\n%s\r\n\r\n' % (req, cookie) )

    def handle_read (self):
        global delay
        data = string.strip( self.recv (8192) )
        print 'Recv: %s' % data
        if data.isdigit():  # only accepts integers 
          print 'input: %s' % data
          delay=float(data)

        self.send ('%s HTTP/1.0\r\n%s\r\n\r\n' % (req, cookie) )

    def handle_write (self):
        pass

    def handle_close (self):
        self.close() 

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
        post_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        post_sock.connect((host, port))
	self.sensor.set_data( random.random()*30 + 273 )   # simulate temperature in K
        print '%s reading: lat: %.1f lon: %.1f data: %.1f' % self.sensor.get_data()
        req= post % self.sensor.get_data() 
        post_sock.send('%s HTTP/1.0\r\n%s\r\n\r\n' % (req, cookie) )
        msg = post_sock.recv(8092)
	#print 'Server response:' , msg
        post_sock.close()

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
    cookie='Cookie: %s\r\nConnection: keep-alive' % ssid
    return cookie

if __name__ == '__main__':
    import sys
    import random
    import urlparse

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
	   time.sleep(random.random()*10) # a bit delay
           pods[sid].start()

       while 1:
           http_client (host, path)
           asyncore.loop()
           print 'out of event loop ...'

       for id in range(1, total_sensors+1): 
	   sid = 's%d' % id
           pods[sid].join()

    else: 
	
       print 'Failed to connect to server. quit ...' 



