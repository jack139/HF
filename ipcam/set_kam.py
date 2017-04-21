#!/usr/bin/env python
# -*- coding: utf-8 -*-

import sys
import base64
import urllib2

def xml_put(url, cam_user, cam_pwd, data_xml):
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


def set_stream(cam_ip, cam_user, cam_pwd, stream_id, ch_name): # 101 - 1st stream, 102 - 2nd stream
	if stream_id==101:
		add_str='<videoScanType>progressive</videoScanType>'
		width=1280
		height=720
		bit_rate=512
	elif stream_id==102:
		add_str=''
		width=704
		height=576
		bit_rate=256
		
	print "IP: %s H: %d W: %d R: %d" % (cam_ip, width, height, bit_rate)
	
	if ch_name==None:
		name_str=''
	else:
		name_str='<channelName>%s</channelName>' % ch_name.encode('utf-8')
		
	data=	'<?xml version="1.0" encoding="UTF-8"?><StreamingChannel xmlns="urn:psialliance-org" version="1.0">' \
		'<id>%d</id>' \
		'%s' \
		'<enabled>true</enabled>' \
		'<Transport>' \
		'<rtspPortNo>554</rtspPortNo>' \
		'<maxPacketSize>1000</maxPacketSize>' \
		'<ControlProtocolList>' \
		'<ControlProtocol>' \
		'<streamingTransport>RTSP</streamingTransport>' \
		'</ControlProtocol>' \
		'<ControlProtocol>' \
		'<streamingTransport>HTTP</streamingTransport>' \
		'</ControlProtocol>' \
		'</ControlProtocolList>' \
		'<Unicast>' \
		'<enabled>true</enabled>' \
		'<rtpTransportType>RTP/TCP</rtpTransportType>' \
		'</Unicast>' \
		'<Security>' \
		'<enabled>true</enabled>' \
		'</Security>' \
		'</Transport>' \
		'<Video>' \
		'<enabled>true</enabled>' \
		'<videoInputChannelID>1</videoInputChannelID>' \
		'<videoCodecType>H.264</videoCodecType>' \
		'%s' \
		'<videoResolutionWidth>%d</videoResolutionWidth>' \
		'<videoResolutionHeight>%d</videoResolutionHeight>' \
		'<videoQualityControlType>cbr</videoQualityControlType>' \
		'<constantBitRate>%d</constantBitRate>' \
		'<fixedQuality>100</fixedQuality>' \
		'<maxFrameRate>1500</maxFrameRate>' \
		'<snapShotImageType>JPEG</snapShotImageType>' \
		'<Extensions>' \
		'<selfExt>' \
		'<GovLength>100</GovLength>' \
		'</selfExt>' \
		'</Extensions>' \
		'</Video>' \
		'</StreamingChannel>' % (stream_id, name_str, add_str, width, height, bit_rate)
		
	return xml_put('http://%s/PSIA/Streaming/channels/%d' % (cam_ip, stream_id), cam_user, cam_pwd, data)

def motion_put(cam_ip, cam_user, cam_pwd, do_motion):
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
	return xml_put('http://%s/PSIA/Custom/MotionDetection/1' % cam_ip, cam_user, cam_pwd, data)

if __name__=='__main__':
	if len(sys.argv)<5:
		print "usage: set_kam.py <ip_address> <user> <pwd> <cmd>"
		print "<cmd>:\tset_stream\t- Set stream settings, name is optioned"
		print "\tset_motion_on\t- Set motion detect ON"
		print "\tset_motion_off\t- Set motion detect OFF"
		sys.exit(2)

	kam_ip=sys.argv[1]
	kam_user=sys.argv[2]
	kam_pwd=sys.argv[3]
	kam_cmd=sys.argv[4]

	if kam_ip=="192.168.100.5": kam_name= u"检票入口"
	elif kam_ip=="192.168.100.6": kam_name= u"大厅左侧"
	elif kam_ip=="192.168.100.7": kam_name= u"爆米花吧台"
	elif kam_ip=="192.168.100.8": kam_name= u"售票台右"
	elif kam_ip=="192.168.100.9": kam_name= u"售票台左"
	elif kam_ip=="192.168.100.10": kam_name= u"4楼长走廊"
	elif kam_ip=="192.168.100.11": kam_name= u"大厅右侧"
	elif kam_ip=="192.168.100.12": kam_name= u"检票口"
	elif kam_ip=="192.168.100.13": kam_name= u"4楼走廊3之右"
	elif kam_ip=="192.168.100.14": kam_name= u"4楼WC正对"
	elif kam_ip=="192.168.100.15": kam_name= u"放映厅3"
	elif kam_ip=="192.168.100.16": kam_name= u"放映机3"
	elif kam_ip=="192.168.100.17": kam_name= u"放映机1"
	elif kam_ip=="192.168.100.18": kam_name= u"4楼3之走廊底"
	elif kam_ip=="192.168.100.19": kam_name= u"4楼走廊起点"
	elif kam_ip=="192.168.100.20": kam_name= u"财务室"
	elif kam_ip=="192.168.100.21": kam_name= u"放映机4"
	elif kam_ip=="192.168.100.22": kam_name= u"放映厅4"
	elif kam_ip=="192.168.100.23": kam_name= u"4楼走廊3之左"
	elif kam_ip=="192.168.100.24": kam_name= u"放映厅6"
	elif kam_ip=="192.168.100.25": kam_name= u"放映厅1"
	elif kam_ip=="192.168.100.26": kam_name= u"放映厅5"
	elif kam_ip=="192.168.100.27": kam_name= u"放映机7"
	elif kam_ip=="192.168.100.28": kam_name= u"放映厅2"
	elif kam_ip=="192.168.100.29": kam_name= u"总经理办门口"
	elif kam_ip=="192.168.100.30": kam_name= u"4楼电梯入口"
	elif kam_ip=="192.168.100.31": kam_name= u"4楼楼梯口"
	elif kam_ip=="192.168.100.32": kam_name= u"放映厅7"
	elif kam_ip=="192.168.100.33": kam_name= u"4楼长走廊末端"
	elif kam_ip=="192.168.100.34": kam_name= u"放映机5"
	elif kam_ip=="192.168.100.35": kam_name= u"4楼走廊3之中"
	elif kam_ip=="192.168.100.36": kam_name= u"放映机2"
	elif kam_ip=="192.168.100.37": kam_name= u"放映机6"
	elif kam_ip=="192.168.100.38": kam_name= u"会议室走廊"
	elif kam_ip=="192.168.100.39": kam_name= u"中控室"
	elif kam_ip=="192.168.0.21": kam_name= u"海康test"
	else: kam_name=None
	
	if kam_cmd=='set_stream':
		print "Set 1st stream"
		set_stream(kam_ip, kam_user, kam_pwd, 101, kam_name)
		print "Set 2nd stream"
		set_stream(kam_ip, kam_user, kam_pwd, 102, kam_name)
	elif kam_cmd=='set_motion_on':
		print "Set motion detect ON"
		motion_put(kam_ip, kam_user, kam_pwd, 10)
	elif kam_cmd=='set_motion_off':
		print "Set motion detect OFF"
		motion_put(kam_ip, kam_user, kam_pwd, 0)
