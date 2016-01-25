#!/usr/local/bin/python
##!/opt/ActivePython-2.5/bin/python
# -*- coding: koi8-r -*-
# $Id: bot.py,v 1.2 2006/10/06 12:30:42 normanr Exp $
import sys, time
import random
import xmpp

oldtime = 0 
age = 300  ## how frequent to read the IP file, in seconds
############################ bot logic start #####################################

def messageCB(conn,mess):
    global ipmsg, oldtime, age, logfile

    text=mess.getBody()
    user=mess.getFrom()
    user.lang='en'      # dup
    if text:
     logfile.write( time.asctime() + str(user) + '\n' )   # log user request 
     newtime = time.time()
     if newtime - oldtime > age:    
       ipfile = open('msnip.txt', 'r')
       ipmsg = ipfile.read()
       ipfile.close()

     iplist = ipmsg.splitlines()
     nips = len(iplist)
     random.seed(user)

     mymsg=''
     for id in range(0, 6):
       mymsg = mymsg + "\n" + iplist[random.randint(0, nips-1)]

     conn.send( xmpp.Message( user, mymsg ) )

def presenceCB(conn,presence):
    type=presence.getType()
    user=presence.getFrom()
    if type == "subscribe":
       conn.send(xmpp.Presence(to=user, typ = 'subscribed'))
       conn.send(xmpp.Presence(to=user, typ = 'subscribe'))

############################# bot logic stop #####################################

def StepOn(conn):
    try:
        conn.Process(1)
    except KeyboardInterrupt: return 0
    return 1

def GoOn(conn):
    while StepOn(conn): pass

if len(sys.argv)<4:
    print "Usage: dtw-bot.py username@server.net password logfile"
else:
    jid=xmpp.JID(sys.argv[1])
    user,server,password=jid.getNode(),jid.getDomain(),sys.argv[2]

    logfile=open(sys.argv[3], 'a') 
    # debug version conn=xmpp.Client(server) 
    conn=xmpp.Client(server,debug=[])
    conres=conn.connect()
    if not conres:
        print "Unable to connect to server %s!"%server
        sys.exit(1)
    if conres<>'tls':
        print "Warning: unable to estabilish secure connection - TLS failed!"
    authres=conn.auth(user,password)
    if not authres:
        print "Unable to authorize on %s - check login/password."%server
        sys.exit(1)
    if authres<>'sasl':
        print "Warning: unable to perform SASL auth os %s. Old authentication method used!"%server
    conn.RegisterHandler('message',messageCB)
    conn.RegisterHandler('presence',presenceCB)
    conn.sendInitPresence()
    print "Bot started."
    GoOn(conn)
