#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# 后台BackBone进程，crond调用，每5分钟运行一次
# */5 * * * * /usr/local/nginx/html/ipcam/backbone.py > /tmp/backbone.log
#
# 1. 检查用户的quota, 删除超量照片, 数据盘保留1G空间free
#

import sys
import time, shutil, os
import helper
from config import setting

web_db = setting.db_web
db = web_db

if __name__=='__main__':
	if len(sys.argv)==1:
		print "usage: backbone.py <num of queues>"
		sys.exit(2)

	num_of_queue=int(sys.argv[1])

	print "BACKBONE: %s started" % helper.time_str()

	#
	#检查用户quota
	#
	_ins=0
	
	# 检查硬盘剩余空间
	df0=os.popen("df --block-size=G %s" % setting.snap_store_path).readlines()
	if len(df0)==2: 
		df=df0[1]
		free_space=int(df.strip().split()[3][:-1])
	else:
		# 如果卷名太长，df会有折行，这是对折行的处理
		df=df0[-1]
		free_space=int(df.strip().split()[2][:-1])

	if free_space<setting.free_space:
		# 删除每个相机最早一个时间片数据
		min_time=2100000000/setting.time_span
		for cam_dir in os.listdir(setting.snap_store_path):
			t_dirs=os.listdir("%s/%s" % (setting.snap_store_path, cam_dir))
			t_dirs.sort()
			if len(t_dirs)>0 and t_dirs[0]!='capture':
				if int(t_dirs[0])<min_time:
					# 保存最前的时间，用于删除alert相关
					min_time=int(t_dirs[0])
				try:
					shutil.rmtree('%s/%s/%s' % (setting.snap_store_path, cam_dir, t_dirs[0]))
					print 'remove %s/%s/%s' % (setting.snap_store_path, cam_dir, t_dirs[0])
					_ins+=1
				except OSError, e:
					print "OSError: %s" % e
				
		# 删除超过quota的比较早的alert_queue里的记录和capture文件
		db_alert=db.alert_queue.find({'time': {'$lt': min_time*setting.time_span}})
		if db_alert.count()>0:
			for alert in db_alert:
				try:
					os.remove('%s/%s/capture/%s' % \
						(setting.snap_store_path, alert['cam'], alert['file']))
					print 'remove %s/%s/capture/%s' % \
						(setting.snap_store_path, alert['cam'], alert['file'])
				except OSError, e:
					print "OSError: %s" % e

		db.alert_queue.remove({'time': {'$lt': min_time*setting.time_span}})

	print "QUOTA: free %dG, remove %d" % (free_space, _ins)
	
	#
	#检查recycle
	#
	_ins=0
	db_rey = db.recycle.find()
	if db_rey.count()>0:
		for u in db_rey:
			if u['type']=='cam': # 删除相机后的后处理
				db.alert_queue.remove({'cam':u['cam']})
				db.last_motion.remove({'cam':u['cam']})
				db.dirty_store.remove({'cam':u['cam']})
				db.recycle.remove({'_id':u['_id']})
				try:
					shutil.rmtree( helper.SnapStore(u['cam']).cam_path() )
					_ins+=1
				except OSError, e:
					print "OSError: %s" % e

	print "RECYCLE: remove %d " % _ins

	print "BACKBONE: %s exited" % helper.time_str()
