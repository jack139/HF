#!/usr/bin/env python
# -*- coding: utf-8 -*-

import sys,os,time

if len(sys.argv)<2:
	print "usage: test_snap.py <check|show>"
	sys.exit(2)

kam_cmd=sys.argv[1]

path='/var/data2/snap_store'

a=os.listdir(path)
a.remove('535e1a5c1ecffb2fa372fd7d')	# this is a camera not used in HF system

if kam_cmd=='show' or kam_cmd=='check':
	last_sub=int(time.time()/600)
	for i in a:
		sub='%s/%s' % (path, i)
		b=os.listdir(sub)
		if 'capture' in b:
			b.remove('capture')
		b.sort()
		sub2='%s/%s' % (sub, b[-1])
		c=os.listdir(sub2)
		if kam_cmd=='show' or last_sub-int(b[-1])>3:
			print "%s - %d, %s - %d, (%d)" % (i, len(b), b[-1], len(c), last_sub-int(b[-1]))
else:
	print "usage: test_snap.py <check|show>"
	sys.exit(2)
	
