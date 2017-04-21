#!/usr/bin/env python
# -*- coding: utf-8 -*-
#

import sys
import shutil, os
from config import setting

#{  ObjectId("534ce70e1ecffb0b5565221d"), "192.168.100.15", "放映厅3", "K307PBVQ3GV3" }
#{  ObjectId("534cf1b11ecffb0b5565225d"), "192.168.100.22", "放映厅4", "K307FX2N35KX" }
#{  ObjectId("534cf3c01ecffb0b55652264"), "192.168.100.24", "放映厅6", "K307PTAL9ASM" }
#{  ObjectId("534cf4dc1ecffb0b55652266"), "192.168.100.25", "放映厅1", "K307MJ5542Q9" }
#{  ObjectId("534cf5681ecffb0b55652268"), "192.168.100.26", "放映厅5", "K307AQX7346Q" }
#{  ObjectId("5361b5331ecffb52af0a7b6f"), "192.168.100.28", "放映厅2", "K307XPRYCT86" }
#{  ObjectId("5361b7761ecffb576ab5a5b0"), "192.168.100.32", "放映厅7", "K30758SFKTGB" }


cam_list = [
	"534ce70e1ecffb0b5565221d",
	"534cf1b11ecffb0b5565225d",
	"534cf3c01ecffb0b55652264",
	"534cf4dc1ecffb0b55652266",
	"534cf5681ecffb0b55652268",
	"5361b5331ecffb52af0a7b6f",
	"5361b7761ecffb576ab5a5b0",

	"535e3bc31ecffb2fa372fdde",
	"5361d2f11ecffb0f1c162d8e",
	"535e3ca01ecffb2fa372fde3",
	"535e2eeb1ecffb2fa372fdc2",
	"535e21e81ecffb2fa372fdb4",
	"5347be091ecffb4c55f32b9d",
	"5347c0411ecffb4c55f32ba3",
	"534bab311ecffb50fcbc590e",
	"534ce0b11ecffb0b55652215",
	"534ce4851ecffb0b5565221a",
	"534cecdc1ecffb0b55652233",
	"534cef201ecffb0b55652253",
	"534cefc81ecffb0b55652255",
	"534cf2d81ecffb0b5565225f",
	"5361b4531ecffb0b20361fbd",
	"5361b5d01ecffb0f1c162d86",
	"5361b6691ecffb576ab5a5aa",
	"5361b6f81ecffb576ab5a5ad",
	"5361b8371ecffb576ab5a5b3",
	"5361b8bc1ecffb576ab5a5b6",
	"5361b9c11ecffb52af0a7b72",
	"5361baec1ecffb576ab5a5b9",
	"5361bc3c1ecffb52af0a7b75",
]

if __name__=='__main__':

	parent = setting.snap_store_path

	n=0
	l=0

	for camid in cam_list:
		t_dirs = os.listdir("%s/%s" % (parent, camid))
		t_dirs.sort()
		for d in t_dirs:
			if d=='capture':
				continue
			#shutil.rmtree('%s/%s/%s' % (parent, camid, d))
			#print 'remove %s/%s/%s' % (parent, camid, d)
			#sys.stdout.flush()

			t_files = os.listdir("%s/%s/%s" % (parent, camid, d))
			for f in t_files:
				n += 1
				os.remove('%s/%s/%s/%s' % (parent, camid, d, f))
				#print '%s - %s removed' % (camid, f)
				#sys.stdout.flush()
			os.rmdir('%s/%s/%s' % (parent, camid, d))
			if n/1000>l:
				l = n/1000
				print '%s - %d files removed' % (camid, n)
				sys.stdout.flush()

