#!/usr/bin/python
###################################################################################
#5/23/07 Yudong Tian
#
#thread-http.py:
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

delay=10
host='127.0.0.1' 
port=18080
path='/'
# Sensor planning service (SPS)
req='GET /activemq-web-demo/message/LISW/SPS?timeout=10000&type=topic'
cookie='Cookie: JSESSIONID=1187oko6ookat\r\nConnection: keep-alive'

# Sensor collection service (SCS): station_id|lat|lon|data
post='POST /activemq-web-demo/message/LISW/SCS?type=topic&body=%s|%f|%f|%f'


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


class sensor_thread(threading.Thread):

    def __init__ (self, name='sensorThread'):
	self._stopevent = threading.Event()
	self._sleepperiod = 1.0
	threading.Thread.__init__(self, name=name)

    def run(self):
      import random
      global delay, post
      while not self._stopevent.isSet(): 
        post_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        post_sock.connect((host, port))
	data = random.random()
	lat = random.random()
	lon = random.random()
        print 'Sensor reading: lat: %f lon: %f data: %f' % (lat, lon, data)
        req= post % (sid, lat, lon, data)
        post_sock.send('%s HTTP/1.0\r\n\r\n' % req )
        msg = post_sock.recv(8092)
	#print 'Server response:' , msg
        post_sock.close()

        time.sleep(delay)

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
    sid = msg.split('Set-Cookie: ')[-1].split(';path=')[0]
    print 'Server response:' , sid
    cookie='Cookie: %s\r\nConnection: keep-alive' % sid
    return cookie

if __name__ == '__main__':
    import sys
    import urlparse

    sensor = sensor_thread()

    cookie = get_session()
    if cookie:

       sensor.start()

       while 1:
           http_client (host, path)
           asyncore.loop()
           print 'out of event loop ...'

       sensor.join()

    else: 
	
       print 'Failed to connect to server. quit ...' 



