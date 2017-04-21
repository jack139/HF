#!/usr/bin/env python
# -*- coding: utf-8 -*-

import web
import time
import gc
import os
import base64
import urllib2
from bson.objectid import ObjectId
from config.url import urls
from config import setting
from config.mongosession import MongoStore
import helper
from helper import Logger, time_str

web_db = setting.db_web  # 默认db使用web本地
file_db = web_db

app = web.application(urls, globals())
application = app.wsgifunc()

web.config.session_parameters['cookie_name'] = 'web_session'
web.config.session_parameters['secret_key'] = 'f6102bff8452386b8ca1'
web.config.session_parameters['timeout'] = 864000 # 10 days
web.config.session_parameters['ignore_expiry'] = True

if setting.debug_mode==False:
  ### for production
  session = web.session.Session(app, MongoStore(web_db, 'sessions'), 
                initializer={'login': 0, 'privilege': 0, 'uname':'', 'uid':''})
else:
  ### for staging,
  if web.config.get('_session') is None:
    session = web.session.Session(app, MongoStore(web_db, 'sessions'), 
                initializer={'login': 0, 'privilege': 0, 'uname':'', 'uid':''})
    web.config._session = session
  else:
    session = web.config._session

gc.set_threshold(300,5,5)

##############################################

PRIV_VISITOR = 0b0000  # 0
PRIV_ADMIN   = 0b1000  # 8
PRIV_USER    = 0b0100  # 4
PRIV_KAM     = 0b0010  # 2
PRIV_SHADOW  = 0b0001  # 1

user_level = { PRIV_VISITOR: '访客',
               PRIV_USER: '会员',
               PRIV_ADMIN: '管理员',
               PRIV_KAM: 'Kam相机',
               PRIV_SHADOW: '影子会员',
}

is_mobi=''  # '' - 普通请求，'M' - html5请求

def my_crypt(codestr):
    import hashlib
    return hashlib.sha1("sAlT139-"+codestr).hexdigest()

def my_rand():
    import random
    return ''.join([random.choice('ABCDEFGHJKLMNPQRSTUVWXY23456789') for ch in range(8)])

def my_simple_hid(codestr):
    codebook='forhetodaythatshedshisbloodwithme';
    cc=len(codebook)
    newcode=''
    for i in range(0, len(codestr)):
      newcode+=codebook[ord(codestr[i])%cc]
    return newcode

def logged(privilege = -1):
    if session.login==1:
      if privilege == -1:  # 只检查login, 不检查权限
        return True
      else:
        if int(session.privilege) & privilege: # 检查特定权限
          return True
        else:
          return False
    else:
        return False

def create_render(privilege, plain=False):
    # 禁用手机登录
    #global is_mobi
    # check mobile
    #if helper.detect_mobile():
    #  is_mobi='M'
    #else:
    #  is_mobi=''
    
    if plain: layout=None
    else: layout='layout'
    
    if logged():
        if privilege == PRIV_USER or privilege == PRIV_SHADOW:
            render = web.template.render('templates/user%s' % is_mobi, base=layout)
        elif privilege == PRIV_ADMIN:
            render = web.template.render('templates/admin%s' % is_mobi, base=layout)
        else:
            render = web.template.render('templates/visitor%s' % is_mobi, base=layout)
    else:
        render = web.template.render('templates/visitor%s' % is_mobi, base=layout)

    # to find memory leak
    #_unreachable = gc.collect()
    #print 'Unreachable object: %d' % _unreachable
    #print 'Garbage object num: %s' % str(gc.garbage)

    return render

class Aticle:
    def GET(self):
      render = create_render(PRIV_VISITOR)
      user_data=web.input(id='')
      if user_data.id=='1':
        return render.article_agreement()
      elif user_data.id=='2':
        return render.article_faq()
      else:
        return render.info('不支持的文档查询！', '/')
        
class Login:
    def GET(self):
        if logged():
            render = create_render(session.privilege)
            return render.portal(session.uname, user_level[session.privilege])
        else:
            render = create_render(session.privilege)

            # 强迫使用https登录
            #if web.ctx.has_key('environ'):
            #  if web.ctx.environ.has_key('HTTPS'):
            #    if web.ctx.environ['HTTPS']!='on':
            #      raise web.seeother('https://%s:%d' % (web.ctx.environ['HTTP_HOST'].split(':')[0],setting.https_port))
            #  else:
            #   raise web.seeother('https://%s:%d' % (web.ctx.environ['HTTP_HOST'].split(':')[0],setting.https_port))
            
            db_sys = web_db.user.find_one({'uname':'settings'})
            if db_sys==None:
              signup=0
            else:
              signup=db_sys['signup']
            Logger.uLog(Logger.VISIT, '')
            return render.login(signup)

    def POST(self):
        name0, passwd = web.input().name, web.input().passwd
        
        if name0[-2]=='#':
          shadow = int(name0[-1])
          name = name0[:-2]
        else:
          shadow = 0
          name = name0
                
        db_user=web_db.user.find_one({'uname':name},{'login':1,'passwd':1,'privilege':1,
        	'passwd%d' % shadow :1})
        if db_user!=None and db_user['login']!=0:
          if (shadow==0 and db_user['passwd']==my_crypt(passwd)) or  \
             (shadow>0 and shadow<6 and db_user['passwd%d' % shadow]!='' and db_user['passwd%d' % shadow]==passwd):
                session.login = 1
                session.uname=name0
                session.uid = db_user['_id']
                session.privilege = PRIV_SHADOW if shadow else db_user['privilege']

                render = create_render(session.privilege)
                                
                # 登录后回到http
                #if web.ctx.has_key('environ'):
                #  if web.ctx.environ.has_key('HTTPS'):
                #    if web.ctx.environ['HTTPS']=='on':
                #      raise web.seeother('http://%s:%d' % (web.ctx.environ['HTTP_HOST'].split(':')[0],setting.http_port))

                return render.portal(session.uname, user_level[session.privilege])
        
        session.login = 0
        session.privilege = 0
        session.uname=''
        render = create_render(session.privilege)
        Logger.uLog(Logger.LOGIN_FAIL, name)
        return render.login_error()

class Reset:
    def GET(self):
        session.login = 0
        session.kill()
        render = create_render(session.privilege)
        return render.logout()

class LiveCam2:
    def _get_camlink(self):
      db_cams=file_db.cams.find({'$and':[{'owner': session.uid},{'alive': 1}]},
                                {'cam_name':1,'delay':1,'schedule':1,'owner':1,'file_serv':1,
                                 'cam_ip':1,'cam_user':1,'cam_passwd':1,'shadow':1}
                               ).sort([('cam_name',1)])
      ipcam_links=[]
      if db_cams.count()>0:
        for cam in db_cams:
          if session.privilege == PRIV_SHADOW: # shadow用户权限检查
            if cam['shadow'][int(session.uname[-1])-1]!='X':
              continue

          if cam['delay']>0 and helper.check_schedule(cam['schedule']):
            ipcam_links.append((cam['cam_name'], cam['_id'], float(1.0/cam['delay']), 
                                base64.b64encode("%s:%s" % (cam['cam_user'],cam['cam_passwd'])),
                                cam['cam_ip']))
          else:
            ipcam_links.append((cam['cam_name'], cam['_id'], -1,
                                base64.b64encode("%s:%s" % (cam['cam_user'],cam['cam_passwd'])),
                                cam['cam_ip']))
      return ipcam_links

    def GET(self):
	if logged(PRIV_USER|PRIV_SHADOW):
		user_data=web.input(full='')
		
		if user_data.full=='1':
			render = create_render(session.privilege, plain=True)
			Logger.uLog(Logger.LIVE_CAM, session.uid)
			return render.live3(session.uname, 
				user_level[session.privilege], self._get_camlink())
		else:
			render = create_render(session.privilege)
			Logger.uLog(Logger.LIVE_CAM, session.uid)
			return render.live2(session.uname, 
				user_level[session.privilege], self._get_camlink())
	else:
		raise web.seeother('/')  

class Search:
	def GET(self):
		if logged(PRIV_USER|PRIV_SHADOW):
			render = create_render(session.privilege)
			user_data=web.input(camid='')
			
			camid = user_data.camid
			
			s_str=user_data.st.replace('T',' ').replace('Z','')
			s_time = int(time.mktime(time.strptime(s_str,"%Y-%m-%d %H:%M:%S")))

			e_str=user_data.et.replace('T',' ').replace('Z','')
			e_time = int(time.mktime(time.strptime(e_str,"%Y-%m-%d %H:%M:%S")))

			db_cam=file_db.cams.find_one({'_id': ObjectId(camid)},
						{'owner':1,'file_serv':1,'cam_type':1})
			if db_cam!=None:
				if db_cam['owner']!=session.uid: #检查相机owner与登录id是否一致
					Logger.uLog(Logger.NO_PRIV, session.uid)
					return render.info('无权查看数据！')  
				
				snap_store=helper.SnapStore(camid)
				vdata = snap_store.snap_search(s_time, e_time)

				ret=str(len(vdata))
				for i in vdata:
					st=time.strftime("%Y-%m-%dT%H:%M:%SZ", 
							time.localtime(float(i[0])))
					et=time.strftime("%Y-%m-%dT%H:%M:%SZ", 
							time.localtime(float(i[1])+10.0))
					ret=ret+'|'+st+'|'+et
				#ret=ret+'|'+my_simple_hid(camid)

				web.header("Content-Type", "text/plain") # Set the Header
				#ret="2|" \
				#    "2014-03-29T18:31:26Z|2014-03-29T19:32:54Z|" \
				#    "2014-03-29T20:31:26Z|2014-03-29T21:32:54Z|" \
				##    "my_simple_hid(camid)"
				return ret

			else:
				return render.info('未找到摄像头！')
		else:
			raise web.seeother('/')
  
class Replay:  # 
    def camdata(self, hid2): # hid2==uid
        cam_info=[]

        if hid2=='': 
          return cam_info

        db_cams=file_db.cams.find({'owner': ObjectId(hid2)},{'cam_name':1, 'shadow':1}).sort([('cam_name',1)])
        if db_cams.count()>0:
          for cam in db_cams:
              if session.privilege == PRIV_SHADOW: # shadow用户权限检查
                if cam['shadow'][int(session.uname[-1])-1]!='X':
                  continue
              cam_info.append([cam['cam_name'], str(cam['_id'])])         
        return cam_info

    def GET(self):
      if logged(PRIV_USER|PRIV_SHADOW):
        render = create_render(session.privilege)

        cam_info=self.camdata(str(session.uid))

        if cam_info!=[]:
          return render.replay2(session.uname, user_level[session.privilege], cam_info)
        else:
          return render.info('未找到摄像头！')
      else:
        raise web.seeother('/')
        
class Download:  # 
    def GET(self):
      if logged(PRIV_USER):
        render = create_render(session.privilege)
        
        cam_info=[]
        
        db_cams=file_db.cams.find({'owner': session.uid},{'cam_name':1, 'shadow':1}).sort([('cam_name',1)])
        if db_cams.count()>0:
          for cam in db_cams:
            #if session.privilege == PRIV_SHADOW: # shadow用户权限检查
            #    if cam['shadow'][int(session.uname[-1])-1]!='X':
            #      continue
            cam_info.append([cam['cam_name'], str(cam['_id'])])
          return render.download(session.uname, user_level[session.privilege], cam_info)
        else:
          return render.info('未找到摄像头！')
      else:
        raise web.seeother('/')

    def POST(self):      
      if logged(PRIV_USER|PRIV_SHADOW):
        render = create_render(session.privilege)
        user_data=web.input(camid='')
        
        camid = user_data.camid

        s_str = '%s %s %s %s:%s:00' % (user_data.year_s, user_data.mon_s, user_data.day_s,
                  user_data.hh_s, user_data.mm_s)
        s_time = int(time.mktime(time.strptime(s_str,"%Y %m %d %H:%M:%S")))

        if user_data.camid=='': 
          return render.info('错误的参数！')  

        db_cam=file_db.cams.find_one({'_id': ObjectId(user_data.camid)},{'owner':1,'resolution':1,'cam_type':1})
        if db_cam!=None:
          snap_store=helper.SnapStore(camid)
          vdata = snap_store.snap_download(s_time)
          return render.download2(session.uname, user_level[session.privilege], '/static/tmp/%s' % vdata)
        else:
          return render.info('错误的参数！！')  
      else:
        raise web.seeother('/')

class SettingsCamsList:
    def _get_camlist(self):
      db_cams=file_db.cams.find({'owner': session.uid},
              {'cam_name':1,'heartbeat':1,'delay':1,'cam_ip':1,'motion_detect':1}).sort([('cam_name',1)])
      ipcam_links=[]
      if db_cams.count()>0:
        for cam in db_cams:
          ipcam_links.append((cam['cam_name'], cam['_id'], 
                    cam['heartbeat'], cam['delay'], cam['cam_ip'], cam['motion_detect']))
      return ipcam_links

    def GET(self):
      if logged(PRIV_USER):
        render = create_render(session.privilege)
        return render.settings_cams_list(session.uname, user_level[session.privilege], self._get_camlist(), time.time())
      else:
        raise web.seeother('/')  

class SettingsAddKam:  # 添加Kam相机
    def GET(self):
      if logged(PRIV_USER):
        render = create_render(session.privilege)
        db_cams=file_db.cams.find({'$and': [{'owner':session.uid},{'cam_type':"kam4a"}]},{'_id':1})
        if db_cams.count()>0: hasKam4a=True
        else: hasKam4a=False
        return render.settings_add_kam(session.uname, user_level[session.privilege], hasKam4a)
      else:
        raise web.seeother('/')
        
    def POST(self):
      if logged(PRIV_USER):
        render = create_render(session.privilege)
        user_data=web.input(order='')

        if user_data.order=="kam4a":
          kam_type="kam4a"
          kam_name='新Kam4A相机'
          kam_order=my_rand()
          db_kam=web_db.kam_cam.find_one({'order': kam_order})
          if db_kam==None: 
            web_db.kam_cam.insert({
                                 'order'           : kam_order,
                                 'shop_order'      : "kam4a",
                                 'alone'           : 1,
                                 'state'           : 1,
                                 'kam'             : kam_order,
                                 'file_serv'       : setting.file_serv_list.keys()[0],
                                 'last_shake'      : 0,
                                 'last_init'       : 0,
                                  })
          else: # 串号重复的几率很小，说明RP有问题
            return render.info('系统暂时有点忙，请稍后重新添加。')
        else:
          # 普通Kam相机
          kam_type="kam307"     # 默认为307，海康, conf时会动态修改
          kam_name='新Kam相机'
          kam_order=user_data.order
          
        db_kam=web_db.kam_cam.find_one({'order': kam_order},
                                       {'alone':1,'uid':1,'kam':1,'file_serv':1})
        if db_kam==None:
          Logger.uLog(Logger.KAM_NOT_FOUND, kam_order)
          return render.info('未发现您的Kam相机，请确保Kam相机串号输入正确。')
        
        if db_kam['alone']==0:
          if db_kam['uid']==session.uid:
            return render.info('您的Kam相机已在使用，不能重复添加！')
          else:
            Logger.uLog(Logger.KAM_IS_OTHERS, kam_order)
            return render.info('此Kam相机正在被其他人使用，请确认识别码是否正确！')

        schedule = {'Sun':[],
                    'Mon':[],
                    'Tue':[],
                    'Wed':[],
                    'Thu':[],
                    'Fri':[],
                    'Sat':[],
                   }        
        cam_setting = {             'cam_name'       : kam_name,
                                    'owner'          : session.uid,
                                    'cam_desc'       : '',
                                    'cam_type'       : kam_type,
                                    'cam_ip'         : '',
                                    'cam_user'       : '',
                                    'cam_passwd'     : '',
                                    'delay'          : -1,
                                    'real_delay'     : -1,  # not use
                                    'motion_detect'  : 0,
                                    'resolution'     : 32,  # not use
                                    'alert_mail'     : '',  # not use
                                    'alert_webchat'  : '',  # not use
                                    'alert_sms'      : '',  # not use
                                    'schedule'       : schedule,
                                    'redirect'       : 0,  # not use
                                    'heartbeat'      : 0,
                                    'kam'            : db_kam['kam'],
                                    'file_serv'      : db_kam['file_serv'],
                                    'bill_expired'   : 2100000000, # 'Fri Jul 18 21:20:00 2036'
                                    'bill_delay'     : -1,  # not use
                                    'bill_resolution': 32,  # not use
                                    'bill_hours'     : 24,  # not use
                                    'bill_disk'      : 0,   # not use
                                    'bill_mails'     : 0,   # not use
                                    'local_id'       : '|',  # not use
                                    'local_addr'     : '',
                                    'remote_addr'    : '',
                                    'upnp_addr'      : '',  # not use
                                    'alive'          : 1,
                                    'sync_kam'       : '',
                                    'shadow'         : '-----',
                      }
                       
        new_cam=file_db.cams.insert(cam_setting)

        # update kam_cam list
        web_db.kam_cam.update({'kam': db_kam['kam']}, 
                          {'$set':{'alone' : 0, 
                                   'cam'   : new_cam, 
                                   'uid'   : session.uid,
                                  }})

        #raise web.seeother('/service?camid=%s&guide=1' % new_cam)
        raise web.seeother('/settings?camid=%s&guide=1' % new_cam)
      else:
        raise web.seeother('/')


class SettingsDelCam:
    def GET(self):
      if logged(PRIV_USER):
        render = create_render(session.privilege)
        user_data=web.input(camid='')

        if user_data.camid=='':
          Logger.uLog(Logger.ERR_PARAM, session.uid)
          return render.info('错误的参数！')  

        db_cam=file_db.cams.find_one({'_id': ObjectId(user_data.camid)},
            {'cam_name':1,'owner':1, 'cam_type':1, 'bill_expired':1})
        if db_cam!=None: 
          if db_cam['owner']!=session.uid: # 检查owner是否与login一致
            Logger.uLog(Logger.NO_PRIV, session.uid)
            return render.info('无权查看数据！')            
        else:
          Logger.uLog(Logger.ERR_PARAM, session.uid)
          return render.info('错误的参数！')          
        
        #if db_cam['bill_expired']>time.time():
        #  return render.info('此相机的服务尚未到期，不能删除！')
        
        if db_cam['cam_type'][:3]=='kam': 
          # 删除kam相机的session，阻止put
          db_kam=web_db.kam_cam.find_one({'cam':db_cam['_id']}, {'kam':1})
          if db_kam!=None: 
            web_db.sessions.remove({'uname':db_kam['kam']})
            file_db.sessions.remove({'uname':db_kam['kam']})
          # 释放 kam 相机
          web_db.kam_cam.update({'cam':db_cam['_id']}, {'$set':{'alone':1}})            
          
        # 只删除cams表，其他数据由backbone删除
        file_db.recycle.insert({'type':'cam','cam':db_cam['_id']})
        file_db.cams.remove({'_id':db_cam['_id']})

        raise web.seeother('/settings_cams')
      else:
        raise web.seeother('/')
              
class Settings:        
    def xml_put(self, url, cam_user, cam_pwd, data_xml):
      try:
        opener = urllib2.build_opener(urllib2.HTTPHandler)
        request = urllib2.Request(url, data=data_xml)
        request.add_header('Content-Type', 'application/xml')
        request.add_header('Authorization', 'Basic %s' % base64.b64encode("%s:%s" % (cam_user,cam_pwd)))
        request.get_method = lambda: 'PUT'
        url = opener.open(request)
        print "RESPONSE: %d" % url.getcode()
        return url.getcode()
      except Exception,e: 
        print "%s: %s" % (type(e), str(e))
        return -1


    def motion_put(self, cam_ip, cam_user, cam_pwd, do_motion):
      data= '<?xml version="1.0" encoding="UTF-8"?><MotionDetection xmlns="urn:psialliance-org" version="1.0">' \
            '<id>1</id>' \
            '<enabled>%s</enabled>' \
            '<samplingInterval>2</samplingInterval>' \
            '<startTriggerTime>500</startTriggerTime>' \
            '<endTriggerTime>500</endTriggerTime>' \
            '<regionType>grid</regionType>' \
            '<Grid>' \
            '<rowGranularity>18</rowGranularity>' \
            '<columnGranularity>22</columnGranularity>' \
            '</Grid>' \
            '<Extensions>' \
            '<selfExt>' \
            '<highlightsenabled>false</highlightsenabled>' \
            '</selfExt>' \
            '</Extensions>' \
            '<MotionDetectionRegionList><MotionDetectionRegion><id>1</id><enabled>true</enabled><sensitivityLevel>60</sensitivityLevel><detectionThreshold>50</detectionThreshold>' \
            '<RegionCoordinatesList><RegionCoordinates><positionX>0</positionX><positionY>0</positionY></RegionCoordinates><RegionCoordinates><positionX>22</positionX><positionY>0</positionY></RegionCoordinates><RegionCoordinates><positionX>22</positionX><positionY>18</positionY></RegionCoordinates><RegionCoordinates><positionX>0</positionX><positionY>18</positionY></RegionCoordinates></RegionCoordinatesList>' \
            '</MotionDetectionRegion></MotionDetectionRegionList>' \
            '</MotionDetection>' % ('true' if do_motion>0 else 'false')

      return self.xml_put('http://%s/PSIA/Custom/MotionDetection/1' % cam_ip, cam_user, cam_pwd, data)

    def osd_put(self, cam_ip, cam_user, cam_pwd, osd_name):
      import urllib2
      data= '<?xml version="1.0" encoding="utf-8"?>' \
            '<VideoInputChannel><id>1</id><inputPort>1</inputPort>' \
            '<Extensions><name>%s</name></Extensions>' \
            '</VideoInputChannel>' % osd_name.encode('utf-8')
      self.xml_put('http://%s/PSIA/System/Video/inputs/channels/1' % cam_ip, cam_user, cam_pwd, data)
      
      data='<?xml version="1.0" encoding="UTF-8"?><OSD xmlns="urn:psialliance-org" version="1.0">' \
           '<id>1</id>' \
           '<enabled>true</enabled>' \
           '<videoInputID>1</videoInputID>' \
           '<attribute>4</attribute>' \
           '<fontSize>selfadaption</fontSize>' \
           '<DateTimeOverlay>' \
           '<id>1</id>' \
           '<enabled>true</enabled>' \
           '<positionX>0</positionX>' \
           '<positionY>32</positionY>' \
           '<type>1</type>' \
           '<clockType>24hour</clockType>' \
           '<displayWeek>false</displayWeek>' \
           '</DateTimeOverlay>' \
           '<channelNameOverlay>' \
           '<id>1</id>' \
           '<enabled>true</enabled>' \
           '<positionX>512</positionX>' \
           '<positionY>512</positionY>' \
           '</channelNameOverlay>' \
           '</OSD>'
      return self.xml_put('http://%s/PSIA/Custom/SelfExt/OSD/channels/1' % cam_ip, cam_user, cam_pwd, data)

    def GET(self):
      if logged(PRIV_USER):
        render = create_render(session.privilege)
        user_data=web.input(camid='', guide='0')

        if user_data.camid=='':
          Logger.uLog(Logger.ERR_PARAM, session.uid)
          return render.info('错误的参数！')  

        db_cam=file_db.cams.find_one({'_id': ObjectId(user_data.camid)})
        if db_cam!=None: 
          if db_cam['owner']!=session.uid: # 检查owner是否与login一致
            Logger.uLog(Logger.NO_PRIV, session.uid)
            return render.info('无权查看数据！')
        else:
          Logger.uLog(Logger.ERR_PARAM, session.uid)
          return render.info('错误的参数！')

        db_kam=web_db.kam_cam.find_one({'cam': db_cam['_id']},{'order':1,'last_init':1,'last_shake':1})
        if db_kam==None:
          Logger.uLog(Logger.ERR_PARAM, session.uid)
          return render.info('错误的参数！')  

        remote_addr = ""
        local_admin = False
        if web.ctx.has_key('environ'):
          if web.ctx.environ.has_key('REMOTE_ADDR'):
            remote_addr =  web.ctx.environ['REMOTE_ADDR']

        if len(db_cam['local_addr'])>0 and \
           (remote_addr==db_cam['remote_addr'] or \
            os.path.splitext(remote_addr)[0]==os.path.splitext(db_cam['local_addr'])[0]
           ):
          local_admin = True

        param=(db_cam['_id'],db_cam['cam_type'],db_cam['cam_name'],db_kam['order'],db_cam['alert_mail'],
               db_cam['delay'],db_cam['motion_detect'],db_cam['schedule'],
               db_cam['bill_expired'],db_cam['bill_delay'],db_cam['local_addr'],
               db_cam['heartbeat'], db_cam['bill_resolution'], db_cam['resolution'],
               time_str(db_kam['last_init']), time_str(db_kam['last_shake']), db_cam['upnp_addr'],
               db_cam['cam_ip'],db_cam['cam_user'],db_cam['cam_passwd'],db_cam['alive'],db_cam['sync_kam'])
        
        return render.settings(session.uname, user_level[session.privilege], param, 
                      local_admin, time.time(), user_data.guide)
      else:
        raise web.seeother('/')

    def POST(self):
      if logged(PRIV_USER):
        render = create_render(session.privilege)
        user_data=web.input(camid='', cam_type='', 
                      sun=[], mon=[], tue=[], wed=[], thu=[], fri=[], sat=[], 
                      motion_detect='0', guide='0', cam_ip='', cam_user='', 
                      alert_mail='', cam_passwd='', alive='1', sync_kam='')

        if user_data.camid=='' or user_data.cam_type=='':
          Logger.uLog(Logger.ERR_PARAM, session.uid)
          return render.info('错误的参数！')  

        #if user_data['alert_mail']!='' and helper.validateEmail(user_data['alert_mail'])==0:
        #  return render.info('请输入正确的邮件报警email地址！')  

        if is_mobi=='M':        
          schedule = {'Sun':[int(x) for x in user_data.sun],
                      'Mon':[int(x) for x in user_data.mon],
                      'Tue':[int(x) for x in user_data.tue],
                      'Wed':[int(x) for x in user_data.wed],
                      'Thu':[int(x) for x in user_data.thu],
                      'Fri':[int(x) for x in user_data.fri],
                      'Sat':[int(x) for x in user_data.sat],
                     }
        else:
          schedule = {'Sun':[int(x) for x in user_data.sun[0].split()],
                      'Mon':[int(x) for x in user_data.mon[0].split()],
                      'Tue':[int(x) for x in user_data.tue[0].split()],
                      'Wed':[int(x) for x in user_data.wed[0].split()],
                      'Thu':[int(x) for x in user_data.thu[0].split()],
                      'Fri':[int(x) for x in user_data.fri[0].split()],
                      'Sat':[int(x) for x in user_data.sat[0].split()],
                     }

        delay=float(user_data['delay'])
        if delay>0 and delay<setting.mini_set_delay: delay=setting.mini_set_delay # 限制最小采样间隔
        
        cam_setting = {'$set' :     { 'cam_name'      : user_data['cam_name'],
                                      'cam_ip'        : user_data['cam_ip'],
                                      'cam_user'      : user_data['cam_user'],
                                      'cam_passwd'    : user_data['cam_passwd'],
                                      'delay'         : delay,
                                      'real_delay'    : delay,
                                      'motion_detect' : int(user_data['motion_detect']),
                                      'resolution'    : int(user_data['resolution']),
                                      'alert_mail'    : user_data['alert_mail'],
                                      #'alert_webchat' : user_data['alert_webchat'],
                                      #'alert_sms'     : user_data['alert_sms'],
                                      'schedule'      : schedule,
                                      'alive'         : int(user_data['alive']),
                                      'sync_kam'      : user_data['sync_kam'],
                                    }
                          }
                                            
        # updata 摄像头设置
        file_db.cams.update({'_id' : ObjectId(user_data['camid'])}, cam_setting)

        # 设置相机的移动侦测
        self.motion_put(user_data['cam_ip'], user_data['cam_user'], 
                        user_data['cam_passwd'], int(user_data['motion_detect']))
        # 设置OSD名字
        self.osd_put(user_data['cam_ip'], user_data['cam_user'], 
                        user_data['cam_passwd'], user_data['cam_name'])
        Logger.uLog(Logger.CAM_UPDATE, user_data['camid'])
        return render.info('成功保存，录像和报警设置在3分钟后生效！','/settings_cams', guide=user_data.guide)
      else:
        raise web.seeother('/')

class ShadowSetting:
    def _get_camlist(self, shadow):
	db_user=file_db.user.find_one({'_id': session.uid},{'passwd%d' % shadow:1})
	
	db_cams=file_db.cams.find({'owner': session.uid},{'cam_name':1,'shadow':1}).sort([('_id',1)])
	ipcam_links=[]
	if db_cams.count()>0:
		for cam in db_cams:
			ipcam_links.append((cam['cam_name'], cam['_id'], cam['shadow'][shadow-1]))
	return (ipcam_links, db_user['passwd%d' % shadow])

    def GET(self):
	if logged(PRIV_USER):
		render = create_render(session.privilege)

		user_data=web.input(shadow='0')

		if not user_data.shadow.isdigit():
			return render.info('参数错误！')

		shadow = int(user_data.shadow)

		if shadow<1 or shadow>5:
			return render.info('参数错误！')

		return render.shadow_setting(session.uname, user_level[session.privilege], 
			self._get_camlist(shadow), shadow)
	else:
		raise web.seeother('/')

    def POST(self):
	if logged(PRIV_USER):
		render = create_render(session.privilege)
		user_data=web.input(shadow='0', cam_num='0', passwd2='')

		shadow = int(user_data.shadow)
		cam_num = int(user_data.num)

		for i in range(cam_num):
			db_cam=file_db.cams.find_one({'_id':ObjectId(user_data['camid_%d' % i])},{'shadow':1})
			shadow_setting=db_cam['shadow'][:shadow-1]+user_data['cam_%d' % i]+db_cam['shadow'][shadow:]
			print db_cam, shadow_setting
			file_db.cams.update({'_id':ObjectId(user_data['camid_%d' % i])},
				{'$set':{'shadow':shadow_setting}})

		web_db.user.update({'_id':session.uid},
			{'$set':{'passwd%d' % shadow : user_data['passwd2'].strip()}})

		Logger.uLog(Logger.USER_UPDATE, session.uid)
		return render.info('成功保存！')
	else:
		raise web.seeother('/')

class SettingsUser:
    def _get_settings(self):
      db_user=web_db.user.find_one({'_id':session.uid},{'uname':1,'full_name':1,'passwd2':1})
      return db_user
        
    def GET(self):
      if logged(PRIV_USER):
        render = create_render(session.privilege)
        return render.settings_user(session.uname, user_level[session.privilege], self._get_settings())
      else:
        raise web.seeother('/')

    def POST(self):
      if logged(PRIV_USER):
        render = create_render(session.privilege)
        full_name = web.input().full_name
        passwd2 = web.input().passwd2.strip()
        old_pwd = web.input().old_pwd.strip()
        new_pwd = web.input().new_pwd.strip()
        new_pwd2 = web.input().new_pwd2.strip()
        
        if old_pwd!='':
          if new_pwd=='':
            return render.info('新密码不能为空！请重新设置。')          
          if new_pwd!=new_pwd2:
            return render.info('两次输入的新密码不一致！请重新设置。')
          db_user=web_db.user.find_one({'_id':session.uid},{'passwd':1})
          if my_crypt(old_pwd)==db_user['passwd']:
            web_db.user.update({'_id':session.uid}, 
              {'$set':{'passwd'   : my_crypt(new_pwd),
                       'full_name': full_name,
                       'passwd2'  : passwd2}})
          else:
            return render.info('登录密码验证失败！请重新设置。')
        else:
          web_db.user.update({'_id':session.uid}, {'$set':{'full_name':full_name, 'passwd2':passwd2}})
        
        Logger.uLog(Logger.USER_UPDATE, session.uid)
        return render.info('成功保存！')
      else:
        raise web.seeother('/')

class AlertQuery:  # 
    def GET(self):
      if logged(PRIV_USER|PRIV_SHADOW):
        render = create_render(session.privilege)
        
        cam_info=[]
        
        db_cams=file_db.cams.find({'owner': session.uid},{'cam_name':1, 'shadow':1}).sort([('cam_name',1)])
        if db_cams.count()>0:
          for cam in db_cams:
            if session.privilege == PRIV_SHADOW: # shadow用户权限检查
                if cam['shadow'][int(session.uname[-1])-1]!='X':
                  continue
            db_alert=file_db.alert_queue.find({'cam':cam['_id']},{'time':1})
            if db_alert.count()>0:
              t1=time_str(db_alert.sort([('time',1)])[0]['time'])
              t2=time_str(db_alert.sort([('time',-1)])[0]['time'])
              cam_info.append([cam['cam_name'], db_alert.count(), t1, t2,str(cam['_id']) ])
            else:
              cam_info.append([cam['cam_name'], 0, '', '', str(cam['_id'])])
          return render.alert_query(session.uname, user_level[session.privilege], cam_info)
        else:
          return render.info('未找到摄像头！')
      else:
        raise web.seeother('/')

    def POST(self):
      if logged(PRIV_USER|PRIV_SHADOW):
        render = create_render(session.privilege)
        user_data=web.input(today='')
        
        camid = user_data.camid

        if user_data.today=='1':
          db_alert=file_db.alert_queue.find({'$and': [{'uid':session.uid},
                                                      {'cam':ObjectId(camid)}]},
                                  {'time':1, 'sent':1, 'rms':1, 'type':1}).limit(50).sort([('time',-1)])
        else:
          s_str = '%s %s %s %s:%s:00' % (user_data.year_s, user_data.mon_s, user_data.day_s,
                  user_data.hh_s, user_data.mm_s)
          e_str = '%s %s %s %s:%s:00' % (user_data.year_e, user_data.mon_e, user_data.day_e,
                  user_data.hh_e, user_data.mm_e)
          s_time = int(time.mktime(time.strptime(s_str,"%Y %m %d %H:%M:%S")))
          e_time = int(time.mktime(time.strptime(e_str,"%Y %m %d %H:%M:%S")))
  
          if e_time - s_time > 3600*24:
            return render.info('查询时段不能超过24小时，请重新选择查询时段。')
          
          db_alert=file_db.alert_queue.find({'$and': [{'uid':session.uid},
                                                      {'cam':ObjectId(camid)},
                                                      {'time':{'$lt':e_time}},
                                                      {'time':{'$gt':s_time}}]},
                                  {'time':1, 'sent':1, 'type':1}).limit(50).sort([('time',-1)])
        alert_ids=[]
        if db_alert.count()>0:
          # 准备log清单
          for alert in db_alert:
            alert_ids.append((alert['_id'], time_str(alert['time']), 0, alert['type']))

        Logger.uLog(Logger.ALERT_QUERY, session.uid)
        return render.alert_log(session.uname, user_level[session.privilege], alert_ids)
      else:
        raise web.seeother('/')

class AlertInfo:
    def GET(self):
      if logged(PRIV_USER|PRIV_SHADOW):
        render = create_render(session.privilege)

        user_data=web.input(alertid='')

        db_alert=file_db.alert_queue.find_one({'_id': ObjectId(user_data.alertid)})
        if db_alert!=None: 
          if db_alert['uid']!=session.uid: # 检查owner是否与login一致
            Logger.uLog(Logger.NO_PRIV, session.uid)
            return render.info('无权查看数据！')
          
          db_cam=file_db.cams.find_one({'_id': db_alert['cam']},{'cam_name':1,'file_serv':1})
          tick=str(int(time.time()))
          hid=my_crypt('%s%s' % (str(session.uid),tick))
          return render.alert_info(session.uname, user_level[session.privilege], 
              db_cam['cam_name'], time_str(db_alert['time']), db_alert)
        else:
          Logger.uLog(Logger.ERR_PARAM, session.uid)
          return render.info('错误的参数！')  
      else:
        raise web.seeother('/') 

class Snapshot:
    def GET(self):
      if logged(PRIV_USER|PRIV_SHADOW):
        #render = create_render(PRIV_USER)
        user_data=web.input(snapid='0')

        db_alert=file_db.alert_queue.find_one({'_id': ObjectId(user_data.snapid)})
        if db_alert!=None: 
          if db_alert['uid']!=session.uid: # 检查owner是否与login一致
            Logger.uLog(Logger.NO_PRIV, session.uid)
            return render.info('无权查看数据！')
  
          db_cam=file_db.cams.find_one({'_id': db_alert['cam']},{'cam_name':1})
          try:
            h=open('%s/%s/capture/%s' % (setting.snap_store_path, db_cam['_id'], db_alert['file']), 'rb')
            r_data = h.read()
            h.close()
          except IOError, e:
            print "IOError: %s" % e
            raise web.seeother('/static/ipcam.png')
  
          #web.header("Content-Type", "images/jpeg") # Set the Header
          web.header("Content-Description", "File Transfer")
          web.header("Content-Type", "application/octet-stream")
          web.header('Content-Disposition', 'attachment; filename="%s"' % db_alert['file'])
          web.header("Content-Transfer-Encoding", "binary")
          web.header("Content-Length", "%d" % len(r_data))
          return r_data
        else:
          raise web.seeother('/static/ipcam.png')

########## Admin 功能 ####################################################
class AdminOrder:
    def GET(self):
        if logged(PRIV_ADMIN):
          render = create_render(session.privilege)
          user_data=web.input(cat='0')

          if user_data.cat=='0': # 所以记录
            condi={}
          elif user_data.cat=='1': # 锁定的
            condi={'state':0}
          elif user_data.cat=='2': # 可用的
            condi={'state':1}
          elif user_data.cat=='4': # 以充值的
            condi={'state':-1}
          elif user_data.cat=='3': # 过期的
            condi={'expired':{'$lt':time.time()}}
            
          orders=[]
          db_order=web_db.orders.find(condi,{'order':1,'cash':1}).sort([('_id',1)])
          if db_order.count()>0:
            tt = time.time()
            for u in db_order:
              orders.append((u['order'],u['_id'],u['cash']))
          return render.order(session.uname, user_level[session.privilege], orders, user_data.cat)
        else:
          raise web.seeother('/')

class AdminOrderDel:
    def GET(self):
      if logged(PRIV_ADMIN):
        render = create_render(session.privilege)
        user_data=web.input(oid='')

        if user_data.oid=='':
          Logger.aLog(Logger.A_ERR_PARAM, session.uid)
          return render.info('错误的参数！')  
        
        db_order=web_db.orders.find_one({'_id':ObjectId(user_data.oid)},{'expired':1,'state':1})
        if db_order!=None:
          if (db_order['state']==0 or db_order['state']==-1) and db_order['expired']<time.time():
            web_db.orders.remove({'_id':ObjectId(user_data.oid)})
            return render.info('已删除！','/admin/order')  
          else:
            return render.info('只能删除已过期的，锁定状态或者已充值的订单！') 
        else:
          Logger.aLog(Logger.A_ERR_PARAM, session.uid)
          return render.info('错误的参数！')  
      else:
        raise web.seeother('/')

class AdminOrderSetting:        
    def GET(self):
      if logged(PRIV_ADMIN):
        render = create_render(session.privilege)
        user_data=web.input(oid='')

        if user_data.oid=='':
          Logger.aLog(Logger.A_ERR_PARAM, session.uid)
          return render.info('错误的参数！')  
        
        db_order=web_db.orders.find_one({'_id':ObjectId(user_data.oid)})
        if db_order!=None:
          return render.order_setting(session.uname, user_level[session.privilege], 
                (db_order['_id'],db_order['order'],db_order['shop_order'],db_order['state'],
                 db_order['cash'],time_str(db_order['expired'])))
        else:
          Logger.aLog(Logger.A_ERR_PARAM, session.uid)
          return render.info('错误的参数！')  
      else:
        raise web.seeother('/')

    def POST(self):
      if logged(PRIV_ADMIN):
        render = create_render(session.privilege)
        user_data=web.input(oid='', state='0')
        
        web_db.orders.update({'_id':ObjectId(user_data['oid'])}, 
            {'$set': {'state': int(user_data['state'])}})
        
        Logger.aLog(Logger.A_ORDER_UPDATE, user_data['oid'])
        return render.info('成功保存！','/admin/order')
      else:
        raise web.seeother('/')

class AdminOrderAdd:
    def GET(self):
      if logged(PRIV_ADMIN):
        render = create_render(session.privilege)
        return render.order_new(session.uname, user_level[session.privilege], my_rand())
      else:
        raise web.seeother('/')

    def POST(self):
      if logged(PRIV_ADMIN):
        render = create_render(session.privilege)
        user_data=web.input(shop_order='', cash='0', state='0')
        
        if user_data.shop_order=='':
          return render.info('淘宝订单编号不能为空！')  

        if user_data.cash=='':
          return render.info('金币数量不能为空！')  
        
        db_order=web_db.orders.find_one({'order': user_data['order']})
        if db_order==None:
          web_db.orders.insert({
                               'order'           : user_data['order'],
                               'shop_order'      : user_data['shop_order'],
                               'cash'            : int(user_data['cash']),
                               'state'           : int(user_data['state']),
                               'expired'         : int(time.time())+3600L*24*60, # 60天不充值过期
                              })
          Logger.aLog(Logger.A_ORDER_ADD, user_data['order'])
          return render.info('成功保存！','/admin/order')
        else:
          return render.info('订单串号已存在！请重新添加。')
      else:
        raise web.seeother('/')


class AdminKam:
    def GET(self):
        if logged(PRIV_ADMIN):
            render = create_render(session.privilege)
            user_data=web.input(cat='0')
  
            if user_data.cat=='0': # 所有记录
              condi={}
            elif user_data.cat=='1': # 锁定的
              condi={'state':0}
            elif user_data.cat=='2': # 可用但未绑定的
              condi={'$and':[{'state':1},{'alone':1}]}
            elif user_data.cat=='3': # 已绑定的
              condi={'alone':0}

            kams=[]            
            db_kam=web_db.kam_cam.find(condi,
                {'order':1, 'kam':1, 'file_serv':1, 'shop_order':1}).sort([('_id',1)])
            if db_kam.count()>0:
              for u in db_kam:
                kams.append((u['order'],u['_id'],u['kam'],u['file_serv'],u['shop_order']))
            return render.kam(session.uname, user_level[session.privilege], kams, user_data.cat)
        else:
            raise web.seeother('/')

class AdminKamSetting:        
    def GET(self):
      if logged(PRIV_ADMIN):
        render = create_render(session.privilege)
        user_data=web.input(kid='')

        if user_data.kid=='':
          Logger.aLog(Logger.A_ERR_PARAM, session.uid)
          return render.info('错误的参数！')  
        
        db_kam=web_db.kam_cam.find_one({'_id':ObjectId(user_data.kid)})
        if db_kam!=None:
          return render.kam_setting(session.uname, user_level[session.privilege], 
                (db_kam['_id'],db_kam['order'],db_kam['shop_order'],db_kam['kam'],db_kam['file_serv'],
                 'n/a' if db_kam['last_shake']==0 else time_str(db_kam['last_shake']),
                 db_kam['state'],db_kam['alone']))
        else:
          Logger.aLog(Logger.A_ERR_PARAM, session.uid)
          return render.info('错误的参数！')  
      else:
        raise web.seeother('/')

    def POST(self):
      if logged(PRIV_ADMIN):
        render = create_render(session.privilege)
        user_data=web.input(kid='', state='0')
        
        web_db.kam_cam.update({'_id':ObjectId(user_data['kid'])}, 
            {'$set': {'state': int(user_data['state'])}})
        
        Logger.aLog(Logger.A_KAM_UPDATE, user_data['kid'])
        return render.info('成功保存！','/admin/kam')
      else:
        raise web.seeother('/')

class AdminKamDel:
    def GET(self):
      if logged(PRIV_ADMIN):
        render = create_render(session.privilege)
        user_data=web.input(kid='')

        if user_data.kid=='':
          Logger.aLog(Logger.A_ERR_PARAM, session.uid)
          return render.info('错误的参数！')  
        
        db_kam=web_db.kam_cam.find_one({'_id':ObjectId(user_data.kid)},{'alone':1})
        if db_kam!=None:
          if db_kam['alone']==1:
            web_db.kam_cam.remove({'_id':ObjectId(user_data.kid)})
            return render.info('已删除！','/admin/kam')  
          else:
            return render.info('不能删除正在使用的相机！') 
        else:
          Logger.aLog(Logger.A_ERR_PARAM, session.uid)
          return render.info('错误的参数！')  
      else:
        raise web.seeother('/')

class AdminKamAdd:
    def GET(self):
      if logged(PRIV_ADMIN):
        render = create_render(session.privilege)
        return render.kam_new(session.uname, user_level[session.privilege], 
                  my_rand(),setting.file_serv_list.keys())
      else:
        raise web.seeother('/')

    def POST(self):
      if logged(PRIV_ADMIN):
        render = create_render(session.privilege)
        user_data=web.input(shop_order='', kam='', state='0')
        
        if user_data.shop_order=='':
          return render.info('淘宝订单编号不能为空！')  

        if user_data.kam=='':
          return render.info('硬件串号不能为空！')  
        
        db_kam=web_db.kam_cam.find_one({'order': user_data['order']})
        if db_kam==None:
          web_db.kam_cam.insert({
                               'order'           : user_data['order'],
                               'shop_order'      : user_data['shop_order'],
                               'alone'           : 1,
                               'state'           : int(user_data['state']),
                               'kam'             : user_data['kam'],
                               'file_serv'       : user_data['file_serv'],
                               'last_shake'      : 0,
                               'last_init'       : 0,
                              })
          Logger.aLog(Logger.A_KAM_ADD, user_data['order'])
          return render.info('成功保存！', '/admin/kam')
        else:
          return render.info('相机串号已存在！请重新添加。')
      else:
        raise web.seeother('/')

class AdminUser:
    def GET(self):
        if logged(PRIV_ADMIN):
            render = create_render(session.privilege)

            users=[]            
            db_user=web_db.user.find({'privilege':PRIV_USER},{'uname':1}).sort([('_id',1)])
            if db_user.count()>0:
              for u in db_user:
                users.append([u['uname'],u['_id']])
            return render.user(session.uname, user_level[session.privilege], users)
        else:
            raise web.seeother('/')

class AdminUserSetting:        
    def GET(self):
      if logged(PRIV_ADMIN):
        render = create_render(session.privilege)
        user_data=web.input(uid='')

        if user_data.uid=='':
          Logger.aLog(Logger.A_ERR_PARAM, session.uid)
          return render.info('错误的参数！')  
        
        db_user=web_db.user.find_one({'_id':ObjectId(user_data.uid)})
        if db_user!=None:
          return render.user_setting(session.uname, user_level[session.privilege], 
              db_user, time_str(db_user['time']))
        else:
          Logger.aLog(Logger.A_ERR_PARAM, session.uid)
          return render.info('错误的参数！')  
      else:
        raise web.seeother('/')

    def POST(self):
      if logged(PRIV_ADMIN):
        render = create_render(session.privilege)
        user_data=web.input(uid='')
        
        web_db.user.update({'_id':ObjectId(user_data['uid'])}, 
            {'$set':{'login' : int(user_data['login'])}})
        
        Logger.aLog(Logger.A_USER_UPDATE, user_data['uid'])
        return render.info('成功保存！','/admin/user')
      else:
        raise web.seeother('/')

class AdminUserAdd:        
    def GET(self):
      if logged(PRIV_ADMIN):
        render = create_render(session.privilege)
        return render.user_new(session.uname, user_level[session.privilege])
      else:
        raise web.seeother('/')

    def POST(self):
      if logged(PRIV_ADMIN):
        render = create_render(session.privilege)
        user_data=web.input(uname='', login='0', passwd='')
        
        if user_data.uname=='':
          return render.info('用户名不能为空！')  
        
        db_user=web_db.user.find_one({'uname': user_data['uname']})
        if db_user==None:
          web_db.user.insert({'login'           : int(user_data['login']),
                              'uname'           : user_data['uname'],
                              'full_name'       : '',
                              'privilege'       : PRIV_USER,
                              'passwd'          : my_crypt(user_data['passwd']),
                              'passwd1'         : '',
                              'passwd2'         : '',
                              'passwd3'         : '',
                              'passwd4'         : '',
                              'passwd5'         : '',
                              'cash'            : 0,
                              'cash_all'        : 0,
                              'time'            : time.time(),  # 注册时间
                             })        
          Logger.aLog(Logger.A_USER_ADD, user_data['uname'])
          return render.info('成功保存！','/admin/user')
        else:
          return render.info('用户名已存在！请修改后重新添加。')
      else:
        raise web.seeother('/')

class AdminSysSetting:        
    def GET(self):
      if logged(PRIV_ADMIN):
        render = create_render(session.privilege)
        
        db_sys=web_db.user.find_one({'uname':'settings'})
        if db_sys!=None:
          return render.sys_setting(session.uname, user_level[session.privilege], db_sys)
        else:
          web_db.user.insert({'uname':'settings','signup':0,'login':0})
          Logger.aLog(Logger.A_SYS_FAIL, session.uid)
          return render.info('如果是新系统，请重新进入此界面。','/')  
      else:
        raise web.seeother('/')

    def POST(self):
      if logged(PRIV_ADMIN):
        render = create_render(session.privilege)
        user_data=web.input(signup='0')
        
        web_db.user.update({'uname':'settings'},{'$set':{'signup': int(user_data['signup'])}})
        
        Logger.aLog(Logger.A_SYS_UPDATE, session.uid)
        return render.info('成功保存！','/admin/sys_setting')
      else:
        raise web.seeother('/')

class AdminSelfSetting:
    def _get_settings(self):
      db_user=web_db.user.find_one({'_id':session.uid})
      return db_user
        
    def GET(self):
      if logged(PRIV_ADMIN):
        render = create_render(session.privilege)
        return render.self_setting(session.uname, user_level[session.privilege], self._get_settings())
      else:
        raise web.seeother('/')

    def POST(self):
      if logged(PRIV_ADMIN):
        render = create_render(session.privilege)
        old_pwd = web.input().old_pwd.strip()
        new_pwd = web.input().new_pwd.strip()
        new_pwd2 = web.input().new_pwd2.strip()
        
        if old_pwd!='':
          if new_pwd=='':
            return render.info('新密码不能为空！请重新设置。')
          if new_pwd!=new_pwd2:
            return render.info('两次输入的新密码不一致！请重新设置。')
          db_user=web_db.user.find_one({'_id':session.uid},{'passwd':1})
          if my_crypt(old_pwd)==db_user['passwd']:
            web_db.user.update({'_id':session.uid}, {'$set':{'passwd':my_crypt(new_pwd)}})
            Logger.aLog(Logger.A_SELF_UPDATE, session.uid)
            return render.info('成功保存！','/')
          else:
            Logger.aLog(Logger.A_SELF_FAIL, session.uid)
            return render.info('登录密码验证失败！请重新设置。')
        else:
          return render.info('未做任何修改。')
      else:
        raise web.seeother('/')

class AdminStatus: 
    def GET(self):
      if logged(PRIV_ADMIN|PRIV_USER):
        render = create_render(session.privilege)
                
        uptime=os.popen('uptime').readlines()
        ipcam=os.popen('pgrep -f "uwsgi_80.sock"').readlines()
        rec=os.popen('pgrep -f "rec"').readlines()
        stream=os.popen('pgrep -f "streaming"').readlines()
        sync=os.popen('pgrep -f "sync_kam"').readlines()
        detector=os.popen('pgrep -f "detector.py"').readlines()
        daemon=os.popen('pgrep -f "daemon.py"').readlines()
        detector_log=os.popen('tail %s/detector_hf.log' % setting.logs_path).readlines()
        daemon_log=os.popen('tail %s/daemon_hf.log' % setting.logs_path).readlines()
        backbone_log=os.popen('tail %s/backbone.log' % setting.logs_path).readlines()
        barjs_log=os.popen('tail %s/bar2.js.log' % setting.logs_path).readlines()
        error_log=os.popen('tail %s/error.log' % setting.logs_path).readlines()
        uwsgi_log=os.popen('tail %s/uwsgi_80.log' % setting.logs_path).readlines()
        sync_log=os.popen('tail %s/sync.log' % setting.logs_path).readlines()
        stream_log=os.popen('tail %s/streaming.log' % setting.logs_path).readlines()
        df_data=os.popen('df -h %s' % setting.snap_store_path).readlines()
        smart1=os.popen('smartctl -H /dev/sda|grep -v Copyright|grep -v linux').readlines()
        smart2=os.popen('smartctl -H /dev/sdb|grep -v Copyright|grep -v linux').readlines()
        smart3=os.popen('smartctl -H /dev/sdc|grep -v Copyright|grep -v linux').readlines()
        smart4=os.popen('smartctl -H /dev/sdd|grep -v Copyright|grep -v linux').readlines()
        smart5=os.popen('smartctl -H /dev/sde|grep -v Copyright|grep -v linux').readlines()
        smart6=os.popen('smartctl -H /dev/sdf|grep -v Copyright|grep -v linux').readlines()

        return render.status(session.uname, user_level[session.privilege],
            {
              'uptime'       :  uptime,
              'ipcam'        :  ipcam,
              'rec'          :  rec,
              'stream'       :  stream,
              'sync'         :  sync,
              'detector'     :  detector,
              'daemon'       :  daemon,
              'detector_log' :  detector_log,
              'daemon_log'   :  daemon_log,
              'backbone_log' :  backbone_log,
              'barjs_log'    :  barjs_log,
              'error_log'    :  error_log,
              'uwsgi_log'    :  uwsgi_log,
              'sync_log'     :  sync_log,
              'stream_log'   :  stream_log,
              'df_data'      :  df_data,
              'smart1'       :  smart1,
              'smart2'       :  smart2,
              'smart3'       :  smart3,
              'smart4'       :  smart4,
              'smart5'       :  smart5,
              'smart6'       :  smart6,
            })
      else:
        raise web.seeother('/')

class AdminData: 
    def GET(self):
      if logged(PRIV_ADMIN):
        render = create_render(session.privilege)
        
        db_active=web_db.user.find({'$and': [{'login'     : 1},
                                         {'privilege' : PRIV_USER},
                                        ]},
                                   {'_id':1}).count()
        db_nonactive=web_db.user.find({'$and': [{'login'     : 0},
                                         {'privilege' : PRIV_USER},
                                        ]},
                                   {'_id':1}).count()
        db_admin=web_db.user.find({'privilege' : PRIV_ADMIN}, {'_id':1}).count()
        db_cam=file_db.cams.find({}, {'_id':1})
        db_cams=db_cam.count()

        db_snaps=0
        #for cam in db_cam:
        #  db_snaps = db_snaps + helper.SnapStore(cam['_id']).total_count()
        db_kams=web_db.kam_cam.find({}, {'_id':1}).count()
          
        db_sessions=web_db.sessions.find({}, {'_id':1}).count()
        db_dirty=file_db.dirty_store.find({}, {'_id':1}).count()
        db_sent=file_db.alert_queue.find({'sent':0}, {'_id':1}).count()
        
        db_ulog01=web_db.ulog.find({'msg':Logger.VISIT}, {'_id':1}).count()
        db_ulog02=web_db.ulog.find({'msg':Logger.LOGIN_FAIL}, {'_id':1}).count()
        db_ulog03=web_db.ulog.find({'msg':Logger.LIVE_CAM}, {'_id':1}).count()
        db_ulog04=web_db.ulog.find({'msg':Logger.ERR_PARAM}, {'_id':1}).count()
        db_ulog05=web_db.ulog.find({'msg':Logger.NO_PRIV}, {'_id':1}).count()
        db_ulog06=web_db.ulog.find({'msg':Logger.REPLAY}, {'_id':1}).count()
        db_ulog07=web_db.ulog.find({'msg':Logger.REACH_MAX}, {'_id':1}).count()
        db_ulog08=web_db.ulog.find({'msg':Logger.KAM_NOT_FOUND}, {'_id':1}).count()
        db_ulog09=web_db.ulog.find({'msg':Logger.KAM_IS_OTHERS}, {'_id':1}).count()
        db_ulog10=web_db.ulog.find({'msg':Logger.CAM_UPDATE}, {'_id':1}).count()
        db_ulog11=web_db.ulog.find({'msg':Logger.USER_UPDATE}, {'_id':1}).count()
        db_ulog12=web_db.ulog.find({'msg':Logger.ALERT_QUERY}, {'_id':1}).count()
        db_ulog13=web_db.ulog.find({'msg':Logger.CONF_ALONE_KAM}, {'_id':1}).count()
        db_ulog14=web_db.ulog.find({'msg':Logger.CONF_NEED_SHAKE}, {'_id':1}).count()
        db_ulog15=web_db.ulog.find({'msg':Logger.PUT_NEED_SHAKE}, {'_id':1}).count()
        
        db_alog01=web_db.alog.find({'msg':Logger.A_ERR_PARAM}, {'_id':1}).count()
        db_alog02=web_db.alog.find({'msg':Logger.A_USER_UPDATE}, {'_id':1}).count()
        db_alog03=web_db.alog.find({'msg':Logger.A_USER_ADD}, {'_id':1}).count()
        db_alog04=web_db.alog.find({'msg':Logger.A_SELF_UPDATE}, {'_id':1}).count()
        db_alog05=web_db.alog.find({'msg':Logger.A_SELF_FAIL}, {'_id':1}).count()
        db_alog06=web_db.alog.find({'msg':Logger.A_IO_ERROR}, {'_id':1}).count()

        return render.data(session.uname, user_level[session.privilege],
            {
              'active'       :  db_active,
              'nonactive'    :  db_nonactive,
              'admin'        :  db_admin,
              'snaps'        :  db_snaps,
              'cams'         :  db_cams,
              'kams'         :  db_kams,
              'sessions'     :  db_sessions,
              'dirty'        :  db_dirty,
              'sent'         :  db_sent, 
              'ulog01'       :  db_ulog01,
              'ulog02'       :  db_ulog02,
              'ulog03'       :  db_ulog03,
              'ulog04'       :  db_ulog04,
              'ulog05'       :  db_ulog05,
              'ulog06'       :  db_ulog06,
              'ulog07'       :  db_ulog07,
              'ulog08'       :  db_ulog08,
              'ulog09'       :  db_ulog09,
              'ulog10'       :  db_ulog10,
              'ulog11'       :  db_ulog11,
              'ulog12'       :  db_ulog12,
              'ulog13'       :  db_ulog13,
              'ulog14'       :  db_ulog14,
              'ulog15'       :  db_ulog15,
              'alog01'       :  db_alog01,
              'alog02'       :  db_alog02,
              'alog03'       :  db_alog03,
              'alog04'       :  db_alog04,
              'alog05'       :  db_alog05,
              'alog06'       :  db_alog06,
            })
      else:
        raise web.seeother('/')


########## Kam 相机 ####################################################

class KamHandshake: 
    def GET(self):
        render = web.template.render('templates/kam')
        user_data=web.input(kamid='',init='')
        
        if user_data.kamid=='':
          return render.handshake(0)  

        db_kam=file_db.cams.find_one({'kam':user_data.kamid},{'owner':1})
        if db_kam==None:
          return render.handshake(0)
        else:
          if user_data.init=='1':
            file_db.kam_cam.update({'kam': user_data.kamid}, {'$set':{'last_shake':long(time.time()),
                                                                      'last_init' :long(time.time())}})
          else:
            file_db.kam_cam.update({'kam': user_data.kamid}, {'$set':{'last_shake':long(time.time())}})
          session.login = 1
          session.uname=user_data.kamid
          session.uid = (db_kam['owner'],db_kam['_id'])  # 同时保存uid和cam
          session.privilege = PRIV_KAM
          return render.handshake(1)

class KamConf:   
    def GET(self): # 不需要参数，从session中取得      
      render = web.template.render('templates/kam')
      user_data=web.input(lost='0')
      
      if logged(PRIV_KAM):                  
        db_cam=file_db.cams.find_one({'_id':session.uid[1]},
            {'delay':1,'cam_user':1,'cam_passwd':1,'cam_ip':1,'schedule':1,'motion_detect':1,'cam_type':1})
        if db_cam==None:
          # 未注册，但怎么握手的？相机被删？cam出错？
          Logger.uLog(Logger.CONF_ALONE_KAM, session.uname)
          return render.conf(-2, [])
        else:
          #如果是新相机，新建snap_store的目录，对老相机无作用
          snap_store=helper.SnapStore(db_cam['_id'])
          
          # 记录相机的内网和外网ip
          local_addr = remote_addr = ""
          cam_type = db_cam['cam_type']
          if web.ctx.has_key('environ'):
            #print "test - %s" % str(web.ctx.environ)
            if web.ctx.environ.has_key('HTTP_USER_AGENT'):
              #print "test - %s" % str(web.ctx.environ['HTTP_USER_AGENT'])
              if "KAM4A" in web.ctx.environ['HTTP_USER_AGENT']: 
                cam_type = "kam4a"
              elif "305" in web.ctx.environ['HTTP_USER_AGENT']: 
                cam_type = "kam305"
              elif "306" in web.ctx.environ['HTTP_USER_AGENT']: 
                cam_type = "kam306"
              elif "307" in web.ctx.environ['HTTP_USER_AGENT']: 
                cam_type = "kam307"
            if web.ctx.environ.has_key('HTTP_X_LOCAL_ADDR'):
              local_addr =  web.ctx.environ['HTTP_X_LOCAL_ADDR']
            if web.ctx.environ.has_key('REMOTE_ADDR'):
              remote_addr =  web.ctx.environ['REMOTE_ADDR']

          # 根据schedule控制是否允许上传照片
          if db_cam['delay']>0 and helper.check_schedule(db_cam['schedule']):
            print "test - cam: %s delay: %f" % (str(db_cam['_id']), db_cam['delay'])
            
            file_db.cams.update({'_id':db_cam['_id']}, {'$set':{'heartbeat'  : time.time(),
                                                           'cam_type'   : cam_type,
                                                           'local_addr' : local_addr,
                                                           'remote_addr': remote_addr }})
            return render.conf(0,(
                                  db_cam['delay'],
                                  db_cam['motion_detect'],
                                  db_cam['_id'],
                                  base64.b64encode("%s:%s" % (db_cam['cam_user'],db_cam['cam_passwd'])),
                                  db_cam['cam_ip'],
                                  setting.snap_store_path
                                 ))
          else:
            file_db.cams.update({'_id':db_cam['_id']}, {'$set':{'heartbeat'  : time.time(),
                                                           'cam_type'   : cam_type,
                                                           'local_addr' : local_addr,
                                                           'remote_addr': remote_addr }})
            return render.conf(0,(
                                  -1,
                                  db_cam['motion_detect'],
                                  db_cam['_id'],
                                  base64.b64encode("%s:%s" % (db_cam['cam_user'],db_cam['cam_passwd'])),
                                  db_cam['cam_ip'],
                                  setting.snap_store_path
                                 ))
      else:
        Logger.uLog(Logger.CONF_NEED_SHAKE, '')
        return render.conf(-1, [])

#if __name__ == "__main__":
#    web.wsgi.runwsgi = lambda func, addr=None: web.wsgi.runfcgi(func, addr)
#    app.run()
