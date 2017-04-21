#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
import web
import time, datetime, os
import urllib2
import re
from config import setting
import du_ext

db = setting.db_web

ISOTIMEFORMAT='%Y-%m-%d %X'

reg_b = re.compile(r"(android|bb\\d+|meego).+mobile|avantgo|bada\\/|blackberry|blazer|compal|elaine|fennec|hiptop|iemobile|ip(hone|od)|iris|kindle|lge |maemo|midp|mmp|netfront|opera m(ob|in)i|palm( os)?|phone|p(ixi|re)\\/|plucker|pocket|psp|series(4|6)0|symbian|treo|up\\.(browser|link)|vodafone|wap|windows (ce|phone)|xda|xiino", re.I|re.M)
reg_v = re.compile(r"1207|6310|6590|3gso|4thp|50[1-6]i|770s|802s|a wa|abac|ac(er|oo|s\\-)|ai(ko|rn)|al(av|ca|co)|amoi|an(ex|ny|yw)|aptu|ar(ch|go)|as(te|us)|attw|au(di|\\-m|r |s )|avan|be(ck|ll|nq)|bi(lb|rd)|bl(ac|az)|br(e|v)w|bumb|bw\\-(n|u)|c55\\/|capi|ccwa|cdm\\-|cell|chtm|cldc|cmd\\-|co(mp|nd)|craw|da(it|ll|ng)|dbte|dc\\-s|devi|dica|dmob|do(c|p)o|ds(12|\\-d)|el(49|ai)|em(l2|ul)|er(ic|k0)|esl8|ez([4-7]0|os|wa|ze)|fetc|fly(\\-|_)|g1 u|g560|gene|gf\\-5|g\\-mo|go(\\.w|od)|gr(ad|un)|haie|hcit|hd\\-(m|p|t)|hei\\-|hi(pt|ta)|hp( i|ip)|hs\\-c|ht(c(\\-| |_|a|g|p|s|t)|tp)|hu(aw|tc)|i\\-(20|go|ma)|i230|iac( |\\-|\\/)|ibro|idea|ig01|ikom|im1k|inno|ipaq|iris|ja(t|v)a|jbro|jemu|jigs|kddi|keji|kgt( |\\/)|klon|kpt |kwc\\-|kyo(c|k)|le(no|xi)|lg( g|\\/(k|l|u)|50|54|\\-[a-w])|libw|lynx|m1\\-w|m3ga|m50\\/|ma(te|ui|xo)|mc(01|21|ca)|m\\-cr|me(rc|ri)|mi(o8|oa|ts)|mmef|mo(01|02|bi|de|do|t(\\-| |o|v)|zz)|mt(50|p1|v )|mwbp|mywa|n10[0-2]|n20[2-3]|n30(0|2)|n50(0|2|5)|n7(0(0|1)|10)|ne((c|m)\\-|on|tf|wf|wg|wt)|nok(6|i)|nzph|o2im|op(ti|wv)|oran|owg1|p800|pan(a|d|t)|pdxg|pg(13|\\-([1-8]|c))|phil|pire|pl(ay|uc)|pn\\-2|po(ck|rt|se)|prox|psio|pt\\-g|qa\\-a|qc(07|12|21|32|60|\\-[2-7]|i\\-)|qtek|r380|r600|raks|rim9|ro(ve|zo)|s55\\/|sa(ge|ma|mm|ms|ny|va)|sc(01|h\\-|oo|p\\-)|sdk\\/|se(c(\\-|0|1)|47|mc|nd|ri)|sgh\\-|shar|sie(\\-|m)|sk\\-0|sl(45|id)|sm(al|ar|b3|it|t5)|so(ft|ny)|sp(01|h\\-|v\\-|v )|sy(01|mb)|t2(18|50)|t6(00|10|18)|ta(gt|lk)|tcl\\-|tdg\\-|tel(i|m)|tim\\-|t\\-mo|to(pl|sh)|ts(70|m\\-|m3|m5)|tx\\-9|up(\\.b|g1|si)|utst|v400|v750|veri|vi(rg|te)|vk(40|5[0-3]|\\-v)|vm40|voda|vulc|vx(52|53|60|61|70|80|81|83|85|98)|w3c(\\-| )|webc|whit|wi(g |nc|nw)|wmlb|wonu|x700|yas\\-|your|zeto|zte\\-", re.I|re.M)

def time_str(t=None):
    return time.strftime(ISOTIMEFORMAT, time.localtime(t))

def check_lan_ip(addr):
  # 10.0.0.0    - 10.255.255.255
  # 172.16.0.0  - 172.31.255.255
  # 192.168.0.0 - 192.168.255.255
    addr2=addr.split('.')
    if len(addr2)!=4: return False
    if addr2[0]=='10': return True
    if addr2[0]=='192' and addr2[1]=='168': return True
    if addr2[0]=='172':
      if addr2[1].isdigit():
        if int(addr2[1])>=16 and int(addr2[1])<=31:
          return True
    return False

def check_schedule(schedule):
    current_datetime = datetime.datetime.now()
    hours = schedule[current_datetime.strftime('%a')]
    if int(current_datetime.strftime('%H')) in hours:
        return True
    else:
        return False

def detect_mobile():
  if web.ctx.has_key('environ'):
    user_agent = web.ctx.environ['HTTP_USER_AGENT']
    b = reg_b.search(user_agent)
    v = reg_v.search(user_agent[0:4])
    if b or v:
      return True
  return False

def validateEmail(email):
    if len(email) > 7:
      if re.match("^.+\\@(\\[?)[a-zA-Z0-9\\-\\.]+\\.([a-zA-Z]{2,3}|[0-9]{1,3})(\\]?)$", email) != None:
        return 1
    return 0

class SmartRedirectHandler(urllib2.HTTPRedirectHandler):
    def http_error_301(self, req, fp, code, msg, headers):
        result = urllib2.HTTPRedirectHandler.http_error_301(
            self, req, fp, code, msg, headers)
        result.status = code
        return result

    def http_error_302(self, req, fp, code, msg, headers):
        result = urllib2.HTTPRedirectHandler.http_error_302(
            self, req, fp, code, msg, headers)
        result.status = code
        return result


###### snap store related #######################################################

class SnapStore:
  def __init__(self, camid):
	self.cam = str(camid)
	self.root = setting.snap_store_path
	self.span = setting.time_span
	self._path = '%s/%s' % (self.root, self.cam)
	if not os.path.exists(self._path):
		os.makedirs(self._path)
		os.chmod(self._path, 0777)

  def cam_path(self):
	return self._path
      
  def snap_path(self, tick):   # 每span秒新建一个目录
    to_path='%s/%d' % (self._path, int(tick)/self.span)
    if not os.path.exists(to_path):
      os.makedirs(to_path)
      os.chmod(to_path, 0777)
    return to_path
  
  def snap_file(self, tick):
    return '%s/%.4f' % (self.snap_path(tick), tick)

  def snap_find(self, st, et=''):
    st_i = int(st)
    if et=='':
      et_i = st_i + 300  # 只返回5分钟
    else:
      et_i = int(et)
    
    st_dir = st_i/self.span
    et_dir = et_i/self.span

    cam_path = os.listdir(self._path)
    # capture 用于存放移动侦测的抓图
    if 'capture' in cam_path:
      cam_path.remove('capture')

    b=[int(i) if int(i)>=st_dir and int(i)<=et_dir else -1 for i in cam_path]
    d=list(set(b))  # 合并重复的 -1
    d.sort()   
    if -1 in d:
      d.remove(-1)  # 去掉 -1
    
    if len(d)==0:
      return []
      
    ret = []
    
    if st_dir==et_dir and len(d)==1:
      l = os.listdir('%s/%d' % (self._path, d[0]))
      for i in l:
        if float(i)>st_i and float(i)<et_i:
          ret.append(i)
    else:    
      for j in d:
        l = os.listdir('%s/%d' % (self._path, j))
        if j==st_dir:
          for i in l:
            if float(i)>st_i:
              ret.append(i)
        elif j==et_dir:
          for i in l:
            if float(i)<et_i:
              ret.append(i)
        elif j>st_dir and j<et_dir:
          ret = ret + l    
    ret.sort()
    return ret

  def snap_download(self, st):
	flist = self.snap_find(st, st+120)  # 下载1分钟的录像
	down_name='download_%s.avi' % time.strftime('%Y-%m-%d_%X', time.localtime())
	down_file=open('%s/%s' % (setting.tmp_path, down_name), 'wb')
	for f in flist:
		data=self.read_snap(float(f))
		down_file.write(data)
	down_file.close()
	return down_name

  def snap_search(self, st, et): # 查询录像时间段  	
	st_i = int(st)
	et_i = int(et)

	st_dir = st_i/self.span
	et_dir = et_i/self.span

	cam_path = os.listdir(self._path)
	# capture 用于存放移动侦测的抓图
	if 'capture' in cam_path:
		cam_path.remove('capture')
    
	b=[int(i) if int(i)>=st_dir and int(i)<=et_dir else -1 for i in cam_path]
	d=list(set(b))  # 合并重复的 -1
	d.sort()   
	if -1 in d:
		d.remove(-1)  # 去掉 -1    

	if len(d)==0:
		return []

	ret=[]
	st=et=''
	last_t=0
	for l in d:
		d2 = os.listdir('%s/%d' % (self._path, l))
		d2.sort()
		for i in d2:
			if st=='':
				st=i
			elif float(i)-float(last_t)>15: # diff less than 15 sec
				et=last_t
				ret.append((st,et))
				st=et=''
				st=i
			last_t=i
	if et=='':
		et=last_t
		ret.append((st,et))
	
	return ret
    
  def write_snap(self, tick, data):
    h=open(self.snap_file(tick), 'wb')
    h.write(data)
    h.close()

  def read_snap(self, tick):
    h=open(self.snap_file(tick), 'rb')
    data = h.read()
    h.close()
    return data
    
  def total_count(self):
    return du_ext.fc(self._path)

  def total_usage(self):
    return du_ext.du(self._path)

  def last_snap_time(self):
    cam_path = os.listdir(self._path)
    if len(cam_path)==0: # NO snapshot
      return 0
    cam_path.sort()
    snap_path = os.listdir('%s/%s' % (self._path, cam_path[-1]))
    snap_path.sort()
    if len(snap_path)>0:
      return float(snap_path[-1])
    else:
      if len(cam_path)>1:
        snap_path = os.listdir('%s/%s' % (self._path, cam_path[-2]))
        snap_path.sort()
        if len(snap_path)>0:
          return float(snap_path[-1])
      return 0

  def first_snap_time(self):
    cam_path = os.listdir(self._path)
    if len(cam_path)==0: # NO snapshot
      return 0
    cam_path.sort()
    snap_path = os.listdir('%s/%s' % (self._path, cam_path[0]))
    snap_path.sort()
    if len(snap_path)>0:
      return float(snap_path[0])
    else:
      if len(cam_path)>1:
        snap_path = os.listdir('%s/%s' % (self._path, cam_path[1]))
        snap_path.sort()
        if len(snap_path)>0:
          return float(snap_path[0])
      return 0
  
  def mean_delay(self, delay):
    cam_path = os.listdir(self._path)
    if len(cam_path)==0:  # NO snapshot
      return delay    
    cam_path.sort()
    snap_path = os.listdir('%s/%s' % (self._path, cam_path[-1]))
    snap_path.sort()
    last_10 = snap_path[-30:]
    if len(last_10)<3:
      return delay
    #计算实际的delay
    snap_tick=0.0
    last_tick=None
    for i in last_10:
      if last_tick==None:
        last_tick=float(i)
        continue
      tick=float(i)
      snap_tick+=abs(tick-last_tick)
      last_tick=tick
    return snap_tick/(len(last_10)-1)
    
###### logger class #######################################################

class Logger:

  VISIT          = 1 # 'Login: visit'
  LOGIN_FAIL     = 2 # 'Login: fail'
  LIVE_CAM       = 3 # 'LiveCam2: '
  ERR_PARAM      = 4 # 'Error: parameter'
  NO_PRIV        = 5 # 'Error: NO PRIV'
  REPLAY         = 6 # 'Replay: post'
  REACH_MAX      = 7 # 'SettingsAddCam: fail - reach limit'
  KAM_NOT_FOUND  = 8 # 'SettingsAddKam: fail - can not found'
  KAM_IS_OTHERS  = 9 # 'SettingsAddKam: fail - add a kam belong to others'
  CAM_UPDATE     = 10 # 'Settings: update'
  USER_UPDATE    = 11 # 'SettingsUser: update'
  ALERT_QUERY    = 12 # 'AlertQuery: post'
  CONF_ALONE_KAM = 13 # 'KamConf: fail - alone cam want to conf'
  CONF_NEED_SHAKE= 14 # 'KamConf: fail - need handshake'
  PUT_NEED_SHAKE = 15 # 'KamPut: fail - need handshake'
  SIGNIN_WRONG   = 16 # 知道hash，email不对，有问题！
  SIGNUP_IOERR   = 17 # 注册验证码出问题
  SIGNUP_WRONG   = 18 # 有问题！
  SERVICE_WRONG  = 19 # 旧服务未过期，不能重复绑定服务！
  SERVICE_BOND   = 20 # 成功绑定相机服务！
  POINTS_NEED    = 21 # 积分不足，请增加积分后再绑定。
  ORDER_NEED     = 22 # 请输入正确的充值串号！
  ORDER_USED     = 23 # 此串号已使用！
  ORDER_LOCKED   = 24 # 此串号尚未确认收货。请在淘宝确认收货后再使用。
  ORDER_EXPIRED  = 25 # 此串号已过期！
  ORDER_ADDED    = 26 # 充值成功！
  

  A_ERR_PARAM    = 901 # 'Error: parameter'
  A_USER_UPDATE  = 902 # 'AdminUserSettings: post'
  A_USER_ADD     = 903 # 'AdminUserAdd: post'
  A_SELF_UPDATE  = 904 # 'AdminSelfSetting: update'
  A_SELF_FAIL    = 905 # 'AdminSelfSetting: update fail, wrong pwd!'
  A_IO_ERROR     = 906 # 'Detector IOError'
  A_SYS_UPDATE   = 907 # 修改系统设置 成功
  A_SYS_FAIL     = 908 # 修改系统设置 取得settings出问题
  A_ORDER_ADD    = 909 # 新增充值订单
  A_ORDER_UPDATE = 910 # 修改充值订单
  A_KAM_ADD      = 911 # 新增KAM相机
  A_KAM_UPDATE   = 912 # 修改KAM相机

  @classmethod 
  def uLog(self, msg, ref, ref2=None):
      db.ulog.insert({'time'  : time.time(),
                      'msg'   : msg,
                      'ref'   : ref,
                      'ref2'  : ref2,
                      'from'  : web.ctx.ip,
                     })

  @classmethod
  def aLog(self, msg, ref):
      db.alog.insert({'time'  : time.time(),
                      'msg'   : msg,
                      'ref'   : ref,
                      'from'  : web.ctx.ip,
                     })

  @classmethod
  def aLog2(self, msg, ref):
      db.alog.insert({'time'  : time.time(),
                      'msg'   : msg,
                      'ref'   : ref,
                      'from'  : 'localhost',
                     })
