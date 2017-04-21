/* -------------- Collections list ---------------*/
web_db.alog
web_db.kam_cam
web_db.sessions
web_db.ulog
web_db.user
web_db.alert_queue  // 仅用于注册
web_db.orders

file_db.alert_queue
file_db.cams
file_db.dirty_store
file_db.last_motion
file_db.recycle
file_db.sessions_kam  // kam相机sessions
file_db.ulog          // kam相机log
file_db.alog

/* -------------- Indexes ---------------*/

web_db:

db.alert_queue.ensureIndex({hash:1})
db.alert_queue.ensureIndex({cam:1})

db.alog.ensureIndex({msg:1})

db.kam_cam.ensureIndex({cam:1})
db.kam_cam.ensureIndex({kam:1})
db.kam_cam.ensureIndex({order:1})
db.kam_cam.ensureIndex({state:1})
db.kam_cam.ensureIndex({alone:1})

db.order.ensureIndex({order:1})
db.order.ensureIndex({state:1})
db.order.ensureIndex({expired:1})

db.sessions.ensureIndex({session_id:1})

db.ulog.ensureIndex({msg:1})

db.user.ensureIndex({privilege:1})
db.user.ensureIndex({uname:1})
db.user.ensureIndex({login:1,privilege:1})


file_db:

db.alert_queue.ensureIndex({type:1})
db.alert_queue.ensureIndex({cam:1})
db.alert_queue.ensureIndex({sent:1})
db.alert_queue.ensureIndex({uid:1,cam:1})
db.alert_queue.ensureIndex({uid:1,cam:1,time:1})
db.alert_queue.ensureIndex({time:1})

db.alog.ensureIndex({msg:1})

db.cams.ensureIndex({cam_type:1})
db.cams.ensureIndex({owner:1})
db.cams.ensureIndex({motion_detect:1})
db.cams.ensureIndex({cam_type:1,queue:1,next_tick:1})
db.cams.ensureIndex({bill_expired:1})
db.cams.ensureIndex({kam:1})

db.dirty_store.ensureIndex({cam:1})

db.last_motion.ensureIndex({cam:1})

db.sessions.ensureIndex({session_id:1})

db.ulog.ensureIndex({msg:1})

/* -------------- orders ---------------*/
{
  order: 'I8AJNQ7K',  // 充值串号
  shop_order: '446965441700053',  // 淘宝订单编号
  cash: 100, // 充值金币数
  state: 1, // 状态：0 - 锁定，已付款，未收货  1 - 可用，已确认收货，-1 -- 已充值
  expired: 1378182973.853102,  // 过期时间，充值后为充值时间
  owner : ObjectId("51db7f81eba46df77f818f81"), // 充值用户
}


/* -------------- cams ---------------*/

{
  cam_name: 'cam1',
  cam_desc: '用于本地测试',
  cam_type: 'cam',                   // cam -- 普通相机   kam -- Kam相机
  cam_ip: 'jsbc.dlinkddns.com.cn',   // kam相机无此项
  cam_port: '8888',                  // kam相机无此项
  cam_user: 'jack',                  // kam相机无此项
  cam_passwd: '13194084665',         // kam相机无此项
  delay: 0.2,                        // 0.2-min -- 采集间隔（秒）, -1 -- 不采样
  real_delay : 0.0697,
  motion_detect: 1,                  // 0 -- 不进行检测, 1 -- 进行移动检测
  resolution: 8,
  alert_mail: 'jack139@gmail.com',
  alert_webchat: '',
  alert_sms: '',
  redirect : 0,
  retry : 0,
  heartbeat : 1378182973.853102, // 最近一次心跳
  kam: "78A5DD03D2A7", // 相机硬件序列号, cam相机无此项
  owner : ObjectId("51db7f81eba46df77f818f81"),
  schedule: '', //{'Mon': [0,1,2,3,4,5,6,7,8,19,20,21,22,23], 表示对应数字的时间进行录像
                // 'Tue': [0,1,2,3,4,5,6,7,8,19,20,21,22,23],
                // 'Wed': [0,1,2,3,4,5,6,7,8,19,20,21,22,23],
                // 'Thu': [0,1,2,3,4,5,6,7,8,19,20,21,22,23],
                // 'Fri': [0,1,2,3,4,5,6,7,8,19,20,21,22,23],
                // 'Sat': [0,1,2,3,4,5,6,7,8,19,20,21,22,23],
                // 'Sun': [0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23],}

  bill_delay: 1,      // 绑定服务 - 录像延迟
  bill_resolution: 8, // 绑定服务 - 视频分辨率
  bill_hours: 12,     // 绑定服务 - 录像保存时间（小时）
  bill_disk: 10,      // 绑定服务 - 录像保存存储大小（M）
  bill_mails: 10,     // 绑定服务 - 每天发邮件数
  bill_expired: 1378182973.853102, // 绑定服务 - 到期时间, 0 - 未绑定服务，不可用
  file_serv: '',
  local_id: '|',
  local_addr: '192.168.1.103',
  remote_addr: '192.168.1.107',
}

/* -------------- kam_cam ---------------*/
{
  order: 'AAU6HOU9', // 相机串号
  shop_order: '446965441700053',  // 淘宝订单编号
  kam: "78A5DD03D2A7", // 相机硬件序列号
  uid: ObjectId("51db7f81eba46df77f818f81"),
  cam: ObjectId("51dd2571ec1e6cb75b0c129d"),
  alone: 0,  // 0 -- 已关联cams，1 -- 未关联，此时cam字段应为空
  state: 1, // 状态：0 - 锁定，已付款，未收货  1 - 可用，已确认收货
  file_serv: '',
  last_shake: 1378185570.408836, // 最近一次握手时间
  last_init: 1378185570.408836, // 最近一次启动时间
}

/* -------------- alert_queue ---------------*/
{ 
  cam : ObjectId("5209a1f017ab2a32fb4f1f60"), 
  curr_time : 1378185571.658591, 
  prev_time : 1378185570.408836, 
  rms : 38.215502828723395, 
  sent : 1, // 0 -- 未发报警， 1 -- 已发报警
  time : 1378185571.658591, 
  type : "motion", // access, motion
  uid : ObjectId("5202084717ab2a4ca39266b5") 
}

/* -------------- last_motion ---------------*/
{
  cam: ObjectId("51db7fb3eba46df77f818f83"),
  snap: ObjectId("51db7fb3eba46df77f818f83"),
  time: 1373354056.953125,  // time.time()
}


/* -------------- ulog ---------------*/
{
  uid: ObjectId("51db7f81eba46df77f818f81"),
  msg: 'login in',
  time: 1373354056.953125,  // time.time()
  ref: '',
}

/* -------------- slog ---------------*/
{
  uid: 'admin',
  msg: 'system backup',
  time: 1373354056.953125  // time.time()
  ref: '',  
}

/* -------------- user ---------------*/
{
	uname : "admin",
	full_name : "jack",
  login: 1,              	  
	privilege : 2,  // 0 -- visitor, 1 -- user, 2 -- admin, 3 -- Kam
	passwd : 'ecb918249fb79775ce1b332ddd2a5d624248ce2b',  // hashlib.sha1("sAlT139-"+passwd).hexdigest()
}

{
  uname:'test',
  full_name:'test test',
  login: 1,              
  privilege: 1, 
  passwd: '26368e2a350d39dfebc3c0d81b13abc51728ed2e',
  limit_max_counts: 4,
  limit_max_quota: 1024, 
  cash: 100, // 可用积分
  cash_all: 0, // 所有积分
}

{
	uname : "settings",
  login: 0,
	privilege : 0,
  mail_smtp_host: 'smtp.qq.com',
  mail_smtp_port: '25',
  mail_smtp_user: '2953116',
  mail_smtp_passwd: '',
  mail_smtp_sender: '2953116@qq.com',
}

/* -------------- First setting ---------------*/
use admin
db.addUser({user:"root",pwd:"root",roles:["userAdmin","userAdminAnyDatabase","readWrite"]})

use web_db
db.addUser({user:"ipcam",pwd:"ipcam",roles:["dbAdmin","readWrite"]})
db.user.insert({uname:"admin",login:1,privilege:8,passwd:'ecb918249fb79775ce1b332ddd2a5d624248ce2b'})

use file_db
db.addUser({user:"ipcam",pwd:"ipcam",roles:["dbAdmin","readWrite"]})
