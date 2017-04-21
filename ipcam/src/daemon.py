#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# 后台daemon进程，启动录像进程，并检查录像进程监控状态
#

import sys
import time, shutil, os
import helper
from config import setting

web_db = setting.db_web
db = web_db

HF_DIR=''
LOG_DIR=''

def start_sync():
	# 准备conf文件
	conf_path = '%s/sync.conf' % HF_DIR
	h=open(conf_path, 'w')
	db_cam=db.cams.find({},{'sync_kam':1})
	if db_cam.count()>0:
		for cam in db_cam:
			if cam['sync_kam'].strip()!='':
				h.write('%s %s\n' % (cam['_id'], 'S307%s' % cam['sync_kam']))
	h.write('$$$\n')
	h.close()

	cmd0="nohup %s/sync_kam %s %s %s >> %s/sync.log &" % \
		(HF_DIR, setting.f8kam_serv, conf_path, setting.snap_store_path, LOG_DIR)
	#print cmd0
	os.system(cmd0)

def start_stream():
	cmd0="nohup %s/streaming 2500 %s >> %s/streaming.log &" % \
		(HF_DIR, setting.snap_store_path, LOG_DIR)
	#print cmd0
	os.system(cmd0)

	cmd0="nohup %s/streaming.old 2501 %s >> %s/streaming.old.log &" % \
		(HF_DIR, setting.snap_store_path, LOG_DIR)
	#print cmd0
	os.system(cmd0)

def start_kam(kamid):
	cmd0="nohup %s/rec %s %s >> %s/rec_%s.log &" % \
		(HF_DIR, setting.web_serv_list['web1'][0], kamid, LOG_DIR, kamid)
	#print cmd0
	os.system(cmd0)
	db.cams.update({'_id':cam['_id']}, {'$set':{'heartbeat'  : time.time()}})

def get_kam_pid(kamid):
	cmd0='pgrep -f "%s"' % kamid
	#print cmd0
	pid=os.popen(cmd0).readlines()
	if len(pid)>0:
		return pid[0].strip()
	else:
		return None

def kill_kam(kamid):
	cmd0='kill -9 `pgrep -f "%s"`' % kamid
	#print cmd0
	os.system(cmd0)

if __name__=='__main__':
	if len(sys.argv)<3:
		print "usage: daemon.py <HF_DIR> <LOG_DIR>"
		sys.exit(2)

	HF_DIR=sys.argv[1]
	LOG_DIR=sys.argv[2]

	print "DAEMON: %s started" % helper.time_str()
	print "HF_DIR=%s\nLOG_DIR=%s" % (HF_DIR, LOG_DIR)

	#
	#启动相机录像进程
	#
	db_cam=db.cams.find({},{'kam':1})
	if db_cam.count()>0:
		for cam in db_cam:
			kill_kam(cam['kam'])
			start_kam(cam['kam'])

	#
	#启动相机回放进程
	#
	kill_kam('%s/streaming' % HF_DIR)
	start_stream()

	#
	#启动相机同步进程
	#
	kill_kam('%s/sync_kam' % HF_DIR)
	start_sync()

	try:	
		_count=_ins=0
		while 1:			
			#
			# 检查Kam相机心跳
			#
			_count+=1
			# 只检查307相机心跳，不检查其他的心跳
			db_cams = db.cams.find({'cam_type':'kam307'}, {'owner':1, 'heartbeat':1, 'kam':1})
			if db_cams.count()>0:
				for cam in db_cams:
					tick=time.time()
					pid=get_kam_pid(cam['kam'])
					if pid==None or tick-cam['heartbeat']>300.0:
						# 进程已死 或 5分钟没有GET conf(心跳)重启进程
						kill_kam(cam['kam'])
						start_kam(cam['kam'])
						_ins+=1
						print "%s\tREC %s restart" % (helper.time_str(), cam['kam'])
			
			# 检查streaming进程
			pid=get_kam_pid('%s/streaming 2500' % HF_DIR)
			pid2=get_kam_pid('%s/streaming.old' % HF_DIR)
			if pid==None or pid2==None:
				# stream进程已死, 重启进程
				kill_kam('%s/streaming' % HF_DIR)
				start_stream()
				_ins+=1
				print "%s\tSTREAMING process restart" % helper.time_str()

			# 检查sync进程
			pid=get_kam_pid('%s/sync_kam' % HF_DIR)
			if pid==None:
				# sync进程已死, 重启进程
				kill_kam('%s/sync_kam' % HF_DIR)
				start_sync()
				_ins+=1
				print "%s\tSYNC process restart" % helper.time_str()

			time.sleep(5)
			if _count>1000:
				if _ins>0:
					print "%s  HEARTBEAT: error %d" % (helper.time_str(), _ins)
				else:
					print "%s  HEARTBEAT: fine." % (helper.time_str())
				_count=_ins=0
			sys.stdout.flush()

	except KeyboardInterrupt:
		print
		print 'Ctrl-C!'

	print "DAEMON: %s exited" % helper.time_str()
