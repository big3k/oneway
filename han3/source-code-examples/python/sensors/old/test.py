
import random

host='%%3C127.0.0.1-%s%%4x' 

print random.random()
print host
print host % 'abc'


data="10.213|3.300"
(clat, clon) =map(float,  data.split("|") )

print "clat=%.1f, clon=%.1f" % (clat, clon)

