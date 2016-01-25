import asyncore
import socket
import string
import threading
import time
import random

delay=10
host='127.0.0.1' 
port=18080
path='/'
# Sensor planning service (SPS)
req='GET /activemq-web-demo/message/LISW/SPS?timeout=10000&type=topic'
cookie='Cookie: JSESSIONID=1187oko6ookat\r\nConnection: keep-alive'

# Sensor collection service (SCS)
post='POST /activemq-web-demo/message/LISW/SCS/?type=topic&body=%%3C%%3Fxml+version%%3D%%221.0%%22+encoding%%3D%%22UTF-8%%22%%3F%%3E%%0D%%0A++%%3Ctitle%%3E%f%%3C%%2Ftitle%%3E%%0D%%0A+'


sockobj = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sockobj.connect((host, port))

req= post % random.random()

sockobj.send ('%s HTTP/1.0\r\n\r\n' % req )
msg = sockobj.recv(8092) 
print 'Server response:' , msg




