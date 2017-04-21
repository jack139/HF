#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# 移动监测进程，后台运行
#
# 1. 检查ftp目录下的文件
# 2. 按归属放入报警队列alert_queue
#

import sys
import gc
import time, shutil, os
import helper
from config import setting

db = setting.db_web


if __name__=='__main__':
  
	print "DETECTOR: %s started" % helper.time_str()

	gc.set_threshold(300,5,5)
  
	try:
		_count=_dir=_ins=0
		while 1:
			# 检查ftp目录
			_count+=1
			ftp_dir=os.listdir(setting.ftp_path)
			for ip in ftp_dir:
				_dir+=1
				db_cam = db.cams.find_one({'cam_ip':ip},{'motion_detect':1,'owner':1})
				
				if db_cam!=None and db_cam['motion_detect']>0:
					if not os.path.exists('%s/%s/capture' % (setting.snap_store_path, db_cam['_id'])):
						os.makedirs('%s/%s/capture' % (setting.snap_store_path, db_cam['_id']))
						os.chmod('%s/%s/capture' % (setting.snap_store_path, db_cam['_id']), 0755)

					# 添加到报警队列
					ip_dir=os.listdir('%s/%s' % (setting.ftp_path, ip))
					ip_dir.sort()
					for jpg in ip_dir:
						# 需要先判断文件是否已写完，先独占open
						ji=jpg.split('_')
						tick=int(time.mktime(time.strptime(ji[2],"%Y%m%d%H%M%S")))
						shutil.move(
							'%s/%s/%s' % (setting.ftp_path, ip, jpg),
							'%s/%s/capture/%s.jpg' % (setting.snap_store_path, db_cam['_id'], ji[2]),
							)

						db.alert_queue.insert({ 
							'uid'	: db_cam['owner'],
							'cam'	: db_cam['_id'],
							'type'	: 'motion',
							'time'	: tick,
							'file'	: '%s.jpg' % ji[2],
							'sent'	: 0,
							})
						#print "ALERT: camid=%s time=%d fn=%s" % (db_cam['_id'],tick,ji[2])
						_ins+=1
				else:
					# 不在相机列表或者未启用移动侦测
					try:
						print "Remove tree: %s" % ip
						shutil.rmtree( '%s/%s' % (setting.ftp_path, ip) )
					except OSError, e:
						print "OS exception: %s" % e
						try:
							print "Remove file: %s" % ip
							os.remove( '%s/%s' % (setting.ftp_path, ip) )
						except OSError, e:
							print "OS exception: %s" % e
			
			time.sleep(1)
			if _count>1000: # 1000
				print "%s  CHECK DIRs: %d  ALERT: %d" % \
					(helper.time_str(), _dir, _ins)
				_count=_dir=_ins=0
			sys.stdout.flush()

	except KeyboardInterrupt:
		print
		print 'Ctrl-C!'
    
	print "DETECTOR: %s exited" % helper.time_str()		
