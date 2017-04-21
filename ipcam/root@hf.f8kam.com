#!/usr/bin/env python
# -*- coding: utf-8 -*-
#

import sys
import shutil, os
from config import setting


if __name__=='__main__':
	if len(sys.argv)==1:
		print "usage: backbone.py <camid>"
		sys.exit(2)

	camid=sys.argv[1]

	parent = setting.snap_store_path

	t_dirs = os.listdir("%s/%s" % (parent, camid))
	t_dirs.sort()
	n=0
	l=0
	for d in t_dirs:
		if d=='capture':
			continue
		t_files = os.listdir("%s/%s/%s" % (parent, camid, d))
		for f in t_files:
			n += 1
			os.remove('%s/%s/%s/%s' % (parent, camid, d, f))
		os.rmdir('%s/%s/%s' % (parent, camid, d))
		if n/10000>l:
			l = n/10000
			print '%s - %d files removed' % (camid, n)
				

