#!/usr/bin/env python

"""Unpack a MIME message into a directory of files."""

import os
import time
import sys
import email
import errno
import mimetypes
from email.Utils import parseaddr


def main():

    # output dir
    odir = '/home/webroot/johnfax/.Fax/emFax/dir1'
    emstr = sys.stdin.read()
    msg = email.message_from_string( emstr )

    ## message debug
    #tfp = open('/tmp/em2.dat', 'wb')
    #tfp.write(emstr) 
    #tfp.close() 
    fromaddr = parseaddr(msg.get("From"))[1]
    faxno = parseaddr(msg.get("To"))[1].split('@')[0]
    # construct basename for fax files  
    basename = time.strftime("%y%m%d%H%M%S", time.localtime()) + \
       '---' + fromaddr + '---' + faxno
    #print "from-to:", basename 
    #print 'Subject:' , msg.get("Subject")
    counter = 0
    numfile = basename + '.txt'
    fp = open(os.path.join(odir, numfile), 'wb')
    fp.write(faxno + '\n')
    fp.close() 

    for part in msg.walk():
      # multipart/* are just containers
      ctype = part.get_content_type()
      if ctype == 'application/octet-stream' \
         or ctype == 'application/msword' \
         or ctype == 'application/pdf' or ctype == 'image/tiff':
        # Applications should really sanitize the given filename so that an
        # email message can't be used to overwrite important files
        filename = part.get_filename()
        if not filename:
            filename = basename + '.fax' 
	else: 
            # rename the file but keep the extension 
	    filename = basename + '.' + filename.split('.')[-1] 

        fp = open(os.path.join(odir, filename), 'wb')
        fp.write(part.get_payload(decode=True))
        fp.close()
	# change owner to johnfax.johnfax
        os.chmod(os.path.join(odir, filename), 0777)


if __name__ == '__main__':
    main()

