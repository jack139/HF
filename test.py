#!/usr/bin/env python
# -*- coding: utf-8 -*-
#

import urllib2

data= '<?xml version="1.0" encoding="UTF-8"?><MotionDetection xmlns="urn:psialliance-org" version="1.0">' \
 '<id>1</id>' \
 '<enabled>true</enabled>' \
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
 '</MotionDetectionRegion></MotionDetectionRegionList></MotionDetection>'

try:
	opener = urllib2.build_opener(urllib2.HTTPHandler)
	request = urllib2.Request('http://192.168.100.20/PSIA/Custom/MotionDetection/1', data=data)
	request.add_header('Content-Type', 'application/x-www-form-urlencoded')
	request.add_header('Authorization', 'Basic YWRtaW46cGNhZG1pbg==')
	request.get_method = lambda: 'PUT'
	opener.open(request, timeout=5)
except Exception,e: 
	print "%s: %s" % (type(e), str(e))
