#!/usr/bin/env python
# -*- coding: utf-8 -*-
import web
from pymongo import MongoClient

#####
debug_mode = True   # Flase - production, True - staging
#####

f8kam_serv='192.168.0.98'

web_serv_list={'web1' : ('192.168.0.98','192.168.0.98')}  # 内网，外网
file_serv_list={'file1': ('192.168.0.98','192.168.0.98')}

local_ip=web_serv_list['web1'][1]

cli = {'web'  : MongoClient(web_serv_list['web1'][0]),
       'file1': MongoClient(file_serv_list['file1'][0]),
      }
      
db_web = cli['web']['hf_db']
db_web.authenticate('ipcam','ipcam')

#db_file1 = cli['file1']['file_db']
#db_file1.authenticate('ipcam','ipcam')

free_space = 10 # G, snap_store 所在卷必须保证的最小可用空间, 单位G
ftp_path = '/home/kamftp'
tmp_path = '/usr/local/nginx/html/ipcam_ws/static/tmp'
logs_path = '/usr/local/nginx/logs'
snap_store_path = '/usr/data3/snap_store'
time_span = 600  # 10 min, snap_store 时间片
mini_delay = 0.01  # for auto setting 
mini_set_delay = 0.25 # for user setting
resolution=8  # 8 - 320*240  32 - 640*480
http_port=80
https_port=443

mail_server='127.0.0.1'
sender='"Kam@Cloud"<kam@f8geek.com>'

web.config.debug = debug_mode

config = web.storage(
    email = 'jack139@gmail.com',
    site_name = 'ipcam',
    site_des = '',
    static = '/static'
)
