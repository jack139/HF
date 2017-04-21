/*****************************************************
Copyright 2007-2011 Hikvision Digital Technology Co., Ltd.   
FileName: timebar
Description: 时间条   不兼容IE浏览器
Author: Chenxiangzhen       
Date: 2011.04.18    
*****************************************************/
/*************************************************
Function:		bindAsEventListener
Description:	绑定对象到函数
Input:			object 对象, fun 函数
Output:			无
return:			关联后的函数
*************************************************/
var bindAsEventListener = function(object, fun) 
{
	var args = Array.prototype.slice.call(arguments).slice(2);
	return function(event) 
	{
		return fun.apply(object, [event || window.event].concat(args));
	}
};
/*************************************************
Function:		bind
Description:	绑定对象到函数
Input:			object 对象, fun 函数
Output:			无
return:			关联后的函数
*************************************************/
var bind = function(object, fun) 
{
	return function() 
	{
		return fun.apply(object, arguments);
	}
};
/*************************************************
Function:		addEventHandler
Description:	添加事件
Input:			oTarget 目标对象, sEventType 事件, fnHandler 函数
Output:			无
return:			无
*************************************************/
var addEventHandler = function(oTarget, sEventType, fnHandler) 
{
	oTarget["on" + sEventType] = fnHandler;
};

var removeEventHandler = function(oTarget, sEventType, fnHandler) 
{
	oTarget["on" + sEventType] = null;
};
/*************************************************
Function:		getObjLeft
Description:	获取对象相对网页的左上角坐标
Input:			obj 对象
Output:			无
return:			坐标
*************************************************/
function getObjLeft(obj)   
{   
	var x = obj.offsetLeft;   
	while(obj=obj.offsetParent) 
	{ 
		x += obj.offsetLeft;
	} 
	return x;   
}

/*************************************************
Function:		getObjTop
Description:	获取对象相对网页的左上角坐标
Input:			obj 对象
Output:			无
return:			坐标
*************************************************/
function getObjTop(obj)   
{   
	var y = obj.offsetTop;   
	while(obj=obj.offsetParent) 
	{ 
		y += obj.offsetTop;
	} 
	return y;   
}
/*************************************************
 * Copyright 2007-2011 Hikvision Digital Technology Co., Ltd. 
 * Class ScaleInfo
 * @author chenxiangzhen
 * @created 2011-04-06
 * @version v1.0
 * @function 工具类，时间刻度信息
 *************************************************/
function ScaleInfo(x, y, iSeconds)
{
	this.m_ix = x;
	this.m_iy = y;
	this.m_ixMin;
	this.m_ixMax;
	this.m_iHour = parseInt(iSeconds / 3600, 10);
	this.m_iMinute = parseInt(iSeconds % 3600 / 60, 10);
	this.m_iSecond = parseInt(iSeconds % 3600 % 60, 10);
	this.m_szTime = "";
	if(this.m_iHour < 10 && this.m_iMinute < 10)
	{
		this.m_szTime = "0" + this.m_iHour + ":0" + this.m_iMinute;
	}
	
	else if(this.m_iHour < 10 && this.m_iMinute >= 10)
	{
		this.m_szTime = "0" + this.m_iHour + ":" + this.m_iMinute;
	}
	
	else if(this.m_iHour >= 10 && this.m_iMinute >= 10)
	{
		this.m_szTime = "" + this.m_iHour + ":" + this.m_iMinute;
	}
	
	else
	{
		this.m_szTime = "" + this.m_iHour + ":0" + this.m_iMinute;
	}
}
/*************************************************
Function:		setPos
Description:	设置刻度的位置
Input:			x 横坐标, y 纵坐标
Output:			无
return:			无
*************************************************/
ScaleInfo.prototype.setPos = function(x, y)
{
	// this.x = x;
	if (x < this.m_ixMin)
	{
		x = this.m_ixMax - (this.m_ixMin - x);
	}
	else if (x > this.m_ixMax)
	{
		x = this.m_ixMin + (x - this.m_ixMax);
	}
	this.m_ix = x;
	this.m_iy = y;
}
/*************************************************
Function:		setPosRange
Description:	设置刻度显示的范围
Input:			ixMin 最小横坐标, ixMax 最大横坐标
Output:			无
return:			无
*************************************************/
ScaleInfo.prototype.setPosRange = function(ixMin, ixMax)
{
	this.m_ixMin = ixMin;
	this.m_ixMax = ixMax;
}
/*************************************************
Function:		isInRange
Description:	是否在范围内
Input:			ixMin 最小横坐标, ixMax 最大横坐标
Output:			无
return:			bool
*************************************************/
ScaleInfo.prototype.isInRange = function(iMin, iMax)
{
	if (this.m_ix >= iMin && this.m_ix <= iMax)
	{
		return true;
	}
	else
	{
		return false;
	}
}
/*************************************************
Function:		update
Description:	更新刻度时间
Input:			iSeconds
Output:			无
return:			无
*************************************************/
ScaleInfo.prototype.update = function(iSeconds)
{
	this.m_iHour = parseInt(iSeconds / 3600, 10);
	this.m_iMinute = parseInt(iSeconds % 3600 / 60, 10);
	this.m_iSecond = parseInt(iSeconds % 3600 % 60, 10);
	if(this.m_iHour < 10 && this.m_iMinute < 10)
	{
		this.m_szTime = "0" + this.m_iHour + ":0" + this.m_iMinute;
	}
	
	else if(this.m_iHour < 10 && this.m_iMinute >= 10)
	{
		this.m_szTime = "0" + this.m_iHour + ":" + this.m_iMinute;
	}
	
	else if(this.m_iHour >= 10 && this.m_iMinute >= 10)
	{
		this.m_szTime = "" + this.m_iHour + ":" + this.m_iMinute;
	}
	
	else
	{
		this.m_szTime = "" + this.m_iHour + ":0" + this.m_iMinute;
	}
}
/*************************************************
 * Copyright 2007-2011 Hikvision Digital Technology Co., Ltd. 
 * Class Time 
 * @author chenxiangzhen
 * @created 2011-04-06
 * @version v1.0
 * @function 工具类，时间相关信息
 *************************************************/
function Time()
{
	var tCurrentTime = new Date();
	this.m_iYear = tCurrentTime.getFullYear();
	this.m_iMonth = tCurrentTime.getMonth()+1;
	this.m_iDay = tCurrentTime.getDate();
	this.m_iHour = tCurrentTime.getHours();
	this.m_iMinute = tCurrentTime.getMinutes();
	this.m_iSecond = tCurrentTime.getSeconds();
	this.m_iMilliseconds = tCurrentTime.getTime();//返回 1970 年 1 月 1 日至今的毫秒数
}
/*************************************************
Function:		setTimeByMis
Description:	设置时间
Input:			iMilliseconds: 1970 年 1 月 1 日至今的毫秒数
Output:			无
return:			无
*************************************************/
Time.prototype.setTimeByMis = function(iMilliseconds)
{
	var tSetTime = new Date(iMilliseconds);
	this.m_iYear = tSetTime.getFullYear();
	this.m_iMonth = tSetTime.getMonth()+1;
	this.m_iDay = tSetTime.getDate();
	this.m_iHour = tSetTime.getHours();
	this.m_iMinute = tSetTime.getMinutes();
	this.m_iSecond = tSetTime.getSeconds();
	this.m_iMilliseconds = iMilliseconds;
}
/*************************************************
Function:		getStringTime
Description:	获取时间字符串
Input:			无
Output:			无
return:			string  yyyy-MM-dd HH:mm:ss
*************************************************/
Time.prototype.getStringTime = function()
{
	var szYear = "" + this.m_iYear;
	
	var szMonth;
	if(this.m_iMonth < 10)
	{
		szMonth = "0" + this.m_iMonth;
	}
	else
	{
		szMonth = "" + this.m_iMonth;
	}
	
	var szDay;
	if(this.m_iDay < 10)
	{
		szDay = "0" + this.m_iDay;
	}
	else
	{
		szDay = "" + this.m_iDay;
	}
	
	var szHour;
	if(this.m_iHour < 10)
	{
		szHour = "0" + this.m_iHour;
	}
	else
	{
		szHour = "" + this.m_iHour;
	}
	
	var szMinute;
	if(this.m_iMinute < 10)
	{
		szMinute = "0" + this.m_iMinute;
	}
	else
	{
		szMinute = "" + this.m_iMinute;
	}
	
	var szSecond;
	if(this.m_iSecond < 10)
	{
		szSecond = "0" + this.m_iSecond;
	}
	else
	{
		szSecond = "" + this.m_iSecond;
	}
	var szCurrentTime = szYear + "-" + szMonth + "-" + szDay + " " + szHour + ":" + szMinute + ":" + szSecond;
	return szCurrentTime;
}
/*************************************************
Function:		parseTime
Description:	通过时间字符串设置时间
Input:			szTime 时间 yyyy-MM-dd HH:mm:ss
Output:			无
return:			无
*************************************************/
Time.prototype.parseTime = function(szTime)
{
	var aDate = szTime.split(' ')[0].split('-');
	var aTime = szTime.split(' ')[1].split(':');
	
	this.m_iYear = parseInt(aDate[0],10);
	this.m_iMonth = parseInt(aDate[1],10);
	this.m_iDay = parseInt(aDate[2],10);
	
	this.m_iHour = parseInt(aTime[0],10);
	this.m_iMinute = parseInt(aTime[1],10);
	this.m_iSecond = parseInt(aTime[2],10);
	
	var tTime = new Date();
	tTime.setFullYear(this.m_iYear);
	tTime.setMonth(this.m_iMonth - 1, this.m_iDay);
	
	tTime.setHours(this.m_iHour);
	tTime.setMinutes(this.m_iMinute);
	tTime.setSeconds(this.m_iSecond);
	
	this.m_iMilliseconds = tTime.getTime();
}
/*************************************************
 * Copyright 2007-2011 Hikvision Digital Technology Co., Ltd. 
 * Class FileInfo 
 * @author chenxiangzhen
 * @created 2011-04-06
 * @version v1.0
 * @function 工具类，录像文件相关信息
 *************************************************/
function FileInfo(iX, iY, iWidth, iHeight, iType, cColor, tStartTime, tStopTime)
{
	this.m_iX = iX;
	this.m_ixMin = 0;
	this.m_ixMax = 0;
	this.m_iY = iY;
	this.m_iWidth = iWidth;
	this.m_iHeight = iHeight;
	this.m_cColor = cColor;
	this.m_iType = iType;
	this.m_tStartTime = tStartTime;
	this.m_tStopTime = tStopTime;
}
/*************************************************
Function:		isInRange
Description:	是否在范围之内
Input:			left 左起始点 right 右终点
Output:			无
return:			无
*************************************************/
FileInfo.prototype.isInRange = function(left, right)
{
	if ((this.m_iX+this.m_iWidth) <= left || this.m_iX >= right)
	{
		return false;
	}
	else
	{
		return true;
	}
}
/*************************************************
Function:		setPos
Description:	设置位置内
Input:			iX iY左起始点坐标 iWidth 宽度 iHeight高度
Output:			无
return:			无
*************************************************/
FileInfo.prototype.setPos = function(iX, iY, iWidth, iHeight)
{
	this.m_iX = iX;
	this.m_iWidth = iWidth;
	this.m_iY = iY;
	this.m_iHeight = iHeight;
}
/*************************************************
Function:		setPosRange
Description:	设置范围
Input:			ixMin, ixMax
Output:			无
return:			无
*************************************************/
FileInfo.prototype.setPosRange = function(ixMin, ixMax)
{
	this.m_ixMin = ixMin;
	this.m_ixMax = ixMax;
}
/*************************************************
Function:		draw
Description:	画文件信息
Input:			g 设备资源
Output:			无
return:			无
*************************************************/
FileInfo.prototype.draw = function(g)
{
	if (this.isInRange(this.m_ixMin, this.m_ixMax))
	{
		var colorOld = g.fillStyle;
		g.fillStyle = this.m_cColor;
		if ((this.m_iX >= this.m_ixMin) && (this.m_iX+this.m_iWidth) <= this.m_ixMax)
		{
			g.fillRect(this.m_iX, this.m_iY, this.m_iWidth, this.m_iHeight);
		}
		else if ((this.m_iX < this.m_ixMax) && ((this.m_iX+this.m_iWidth) > this.m_ixMax))
		{
			g.fillRect(this.m_iX, this.m_iY, this.m_ixMax - this.m_iX, this.m_iHeight);
		}
		else
		{
			g.fillRect(this.m_ixMin, this.m_iY, (this.m_iX+this.m_iWidth) - this.m_ixMin, this.m_iHeight);
		}
		g.fillStyle = colorOld;
	}
}
/*************************************************
 * Copyright 2007-2011 Hikvision Digital Technology Co., Ltd. 
 * Class TimeBar
 * @author chenxiangzhen
 * @created 2011-04-06
 * @version v1.0
 * @function 工具类，时间条
 *************************************************/
function TimeBar(canvas/*, iWidth, iHeight*/)
{  
	if(arguments.length >= 3)
	{
		canvas.width = arguments[1];
		canvas.height = arguments[2];
	}
	else
	{
		canvas.width = 300;
		canvas.height = 300;
	}
	this.m_canvas = canvas;
	this.m_ctx = canvas.getContext("2d");
	
	this.m_iMinFileWidth = 1;                       //文件的最小宽度
	
	this.backgroundColor = 'rgb(0, 0, 0)';          //时间条背景颜色
    this.partLineColor = 'rgb(48,48,48)';          //分割线颜色
    this.channelNameColor = 'rgb(150, 150, 150)';   //通道名称颜色
    this.timeScaleColor = 'rgb(150, 150, 150)';     //时间条刻度颜色
    this.middleLineColor = 'rgb(255, 204, 0)';      //中轴线颜色
    this.middleLineTimeColor = 'rgb(255, 255, 255)';//中轴时间颜色
    this.defaultFileColor = 'rgb(0, 255, 0)';       //默认录像类型颜色
    this.cmdFileColor = 'rgb(21, 184, 155)';         //命令触发录像颜色
    this.scheFileColor = 'rgb(99, 125, 236)';      //录像计划颜色
    this.alarmFileColor = 'rgb(248, 71, 126)';     //警告录像颜色
    this.manualFileColor = 'rgb(247, 199, 5)';      //手动录像颜色
	
	this.m_fMidTimeFont = '14px Verdana';          //中线时间字体及大小
	this.m_fCurTimeFont = '12px Verdana';          //鼠标当前时间字体及大小
	this.m_fScaleFont = '10px Verdana';            //刻度字体及大小  sans-serif
	this.m_fChannelNameFont = '14px Verdana';      //通道名称字体
	
	canvas.style.backgroundColor = this.backgroundColor;
	
	this.m_szCurChannelName = '';  //当前通道名称
	this.m_fCellTime = parseFloat(1.0) //每个代表几个小时
	this.ScaleInfo = new Array();
	this.ScaleInfoNum = parseInt(24/this.m_fCellTime, 10); //总的刻度数量
	this.ScaleInfoDisNum = 12 //显示的刻度数量
	
	//初始化刻度
	for(var i = 0; i < this.ScaleInfoNum; i++) 
	{
		this.ScaleInfo.push(new ScaleInfo(0, 0, parseInt(i * 3600 * this.m_fCellTime)));
	}
	this.m_iMaxWndNum = 16;   //最大窗口数
	this.m_iSelWnd = 0;       //选中的窗口号
	
	this.FileInfoSet = new Array(this.m_iMaxWndNum);  //文件信息集合
	//初始化文件信息集合
	for(i = 0; i < this.m_iMaxWndNum; i++)
	{
		this.FileInfoSet[i] = new Array();
	}
	this.m_iHeight = parseInt(canvas.height, 10);
	this.m_iWidth = parseInt(canvas.width, 10);
	this.m_iFileListStartPos = 0;  // 文件列表起始位置
	this.m_iBlankHeight = 4;    // 中间及底边空白宽度
	this.m_iTimeRectHeight = 40;  //parseInt(this.m_iHeight * 4 / 7)  时间块的高度
	this.m_iFileRectHeight = this.m_iHeight - this.m_iTimeRectHeight - this.m_iBlankHeight * 2; //文件块的高度
	this.m_iMiddleLinePos = parseInt((this.m_iFileListStartPos + this.m_iWidth) / 2, 10);  //中轴线的位置
	this.m_iCellWidth = Math.floor((this.m_iWidth - this.m_iFileListStartPos)/this.ScaleInfoDisNum);  //每个像素的秒数
	this.m_iCellMilliseconds = parseInt((3600 * this.m_fCellTime * 1000)/this.m_iCellWidth, 10);  //每个像素的毫秒数
	
	this.m_tCurrentMidTime = new Time(); //当前中轴线的时间
	this.m_ctx.font = this.m_fMidTimeFont;
	this.m_iTextWidth = this.m_ctx.measureText(this.m_tCurrentMidTime.getStringTime()).width;
	
	this.m_tMouseCurTime = new Time();  //当前鼠标点的时间
	this.m_ctx.font = this.m_fCurTimeFont;
	this.m_iCurTextWidth = this.m_ctx.measureText(this.m_tMouseCurTime.getStringTime()).width;
	
	this.m_iCanvasLeft = getObjLeft(this.m_canvas);
	this.m_iCanvasTop = getObjTop(this.m_canvas);
	
	//初始化时间刻度信息
	for (i = 0; i < this.ScaleInfoNum; i++)
	{
		// 计算与中轴线的时间差（只计算时分秒）
		var seconds = (this.ScaleInfo[i].m_iHour - this.m_tCurrentMidTime.m_iHour) * 3600 + (this.ScaleInfo[i].m_iMinute - this.m_tCurrentMidTime.m_iMinute) * 60 + (this.ScaleInfo[i].m_iSecond - this.m_tCurrentMidTime.m_iSecond);
		var iScalePos = this.m_iMiddleLinePos + parseInt(parseFloat(seconds / (3600*this.m_fCellTime)) * this.m_iCellWidth);
		// 设置刻度位置范围
		this.ScaleInfo[i].setPosRange(this.m_iFileListStartPos, this.m_iFileListStartPos + parseInt(this.m_iCellWidth * this.ScaleInfoNum));
		this.ScaleInfo[i].setPos(iScalePos, this.m_iTimeRectHeight);
	}
	
	//注册消息响应
	this.m_ieventX = 0;
	this.m_iMousePosX = 0;
	
	this.m_bMOuseDown = false;
	this.m_bMouseOver = false;
	this.m_iMove = 0;
	this.m_iMiddleLineTime = 0;
	this.Start = function(oEvent)
	{
		this.m_ieventX = oEvent.clientX
		this.m_iMiddleLineTime = this.m_tCurrentMidTime.m_iMilliseconds;
		this.m_bMOuseDown = true;
		addEventHandler(document, 'mousemove', bindAsEventListener(this, this.Move));  
		addEventHandler(document, 'mouseup', bind(this, this.Stop));
		//焦点丢失
		addEventHandler(window, "blur", bindAsEventListener(this, bindAsEventListener(this, this.Stop)));
		//阻止默认动作
		oEvent.preventDefault();
		
		removeEventHandler(canvas, 'mousemove', bindAsEventListener(this, this.onMouseMove));
	}
	
	this.mouseUpCallbackFunc = function(){};
	this.Stop = function()
	{
		document.body.style.cursor='default';
		this.m_canvas.style.cursor="url(/static/images/playback/H_point1.cur),pointer";
		this.m_bMOuseDown = false;
		this.mouseUpCallbackFunc();
		removeEventHandler(document, 'mousemove', bindAsEventListener(this, this.Move));  
		removeEventHandler(document, 'mouseup', bindAsEventListener(this, this.Stop));
		removeEventHandler(window, "blur", bindAsEventListener(this, this.Stop));
		
		addEventHandler(canvas, 'mousemove', bindAsEventListener(this, this.onMouseMove));
	}
	this.onMouseMoveIn = true;
	this.Move = function(oEvent)
	{
		document.body.style.cursor="url(/static/images/playback/H_point.cur),pointer";
		this.m_canvas.style.cursor="url(/static/images/playback/H_point.cur),pointer";
		this.m_iMove = oEvent.clientX - this.m_ieventX;
		if(this.m_bMOuseDown)
		{
			//清除选择
			window.getSelection ? window.getSelection().removeAllRanges() : document.selection.empty();
			this.m_tCurrentMidTime.setTimeByMis(this.m_iMiddleLineTime - this.m_iMove * this.m_iCellMilliseconds);
			this.repaint();
		}
	}
	this.m_canvas.style.cursor="url(/static/images/playback/H_point1.cur),pointer";
	this.onMouseMove = function(oEvent)
	{
		this.m_iMousePosX = oEvent.clientX - this.m_iCanvasLeft;
		this.m_tMouseCurTime.setTimeByMis((this.m_iMousePosX - this.m_iMiddleLinePos) * this.m_iCellMilliseconds + this.m_tCurrentMidTime.m_iMilliseconds);
		this.repaint();
		var szCurMouseTime = this.m_tMouseCurTime.getStringTime();
		this.m_ctx.fillStyle = this.middleLineTimeColor;
		this.m_ctx.font = this.m_fCurTimeFont;
		this.m_ctx.fillText(szCurMouseTime, (this.m_iMousePosX - parseInt(this.m_iCurTextWidth/2)), parseInt(this.m_iTimeRectHeight/4));
	}
	
	this.onMouseOut = function(oEvent)
	{
		this.repaint();
	}
	addEventHandler(canvas, 'mousedown', bindAsEventListener(this, this.Start));
	
	addEventHandler(canvas, 'mousemove', bindAsEventListener(this, this.onMouseMove));
	addEventHandler(canvas, 'mouseout', bindAsEventListener(this, this.onMouseOut));
	
	//this.setMidLineTime('2011-04-20 10:00:00');
	this.repaint();
}

/*************************************************
Function:		repaint
Description:	重绘
Input:			无
Output:			无
return:			无
*************************************************/
TimeBar.prototype.repaint = function()
{
	var szCurrentTime = this.m_tCurrentMidTime.getStringTime();
	
	this.updateScalePos();
	this.updateFileListPos();
	
	this.m_ctx.clearRect(0, 0, this.m_iWidth, this.m_iHeight);
	
	//画中轴时间
	this.m_ctx.fillStyle = this.middleLineTimeColor;
	this.m_ctx.font = this.m_fMidTimeFont;
	this.m_iTextWidth = this.m_ctx.measureText(szCurrentTime).width;
	this.m_ctx.fillText(szCurrentTime, (this.m_iMiddleLinePos - parseInt(this.m_iTextWidth/2)), parseInt(this.m_iTimeRectHeight/2) + 5);
	
	//画通道名称分割线
	this.m_ctx.strokeStyle = this.partLineColor;
	this.m_ctx.lineWidth = 1;
	this.m_ctx.beginPath();
	this.m_ctx.moveTo(this.m_iFileListStartPos, this.m_iTimeRectHeight);
	this.m_ctx.lineTo(this.m_iFileListStartPos, this.m_iHeight);
	this.m_ctx.stroke();
	
	//画文件两条横轴和纵轴
	this.m_ctx.lineWidth = this.m_iBlankHeight;
	this.m_ctx.beginPath();
	this.m_ctx.moveTo(0, this.m_iTimeRectHeight);
	this.m_ctx.lineTo(this.m_iWidth, this.m_iTimeRectHeight);
	this.m_ctx.stroke();
	
	this.m_ctx.beginPath();
	this.m_ctx.moveTo(0, this.m_iHeight - this.m_iBlankHeight/2);
	this.m_ctx.lineTo(this.m_iWidth, this.m_iHeight - this.m_iBlankHeight/2);
	this.m_ctx.stroke();
	
	//显示通道名称
	this.m_ctx.fillStyle = this.channelNameColor;
	this.m_ctx.font = this.m_fChannelNameFont;
	this.m_ctx.fillText(this.m_szCurChannelName, 0, this.m_iTimeRectHeight + this.m_iBlankHeight + parseInt(this.m_iFileRectHeight / 2) + 5, 90);
	
	this.m_ctx.strokeStyle = this.timeScaleColor;
	this.m_ctx.font = this.m_fScaleFont;
	this.m_ctx.lineWidth = 1;
	//画时间刻度
	for (i = 0; i < this.ScaleInfoNum; i++)
	{
		if (this.ScaleInfo[i].isInRange(this.m_iFileListStartPos, this.m_iWidth))
		{
			this.m_ctx.beginPath();
			this.m_ctx.moveTo(this.ScaleInfo[i].m_ix, this.m_iTimeRectHeight);
			this.m_ctx.lineTo(this.ScaleInfo[i].m_ix, this.m_iHeight);
			this.m_ctx.stroke();
			this.m_ctx.fillText(this.ScaleInfo[i].m_szTime, this.ScaleInfo[i].m_ix - 15, this.m_iTimeRectHeight - 5);
		}
	}
	
	//画文件信息区域
	for(i = 0; i < this.FileInfoSet[this.m_iSelWnd].length; i++)
	{
		this.FileInfoSet[this.m_iSelWnd][i].draw(this.m_ctx);
	}
	
	
	//画中轴线
	this.m_ctx.strokeStyle = this.middleLineColor;
	this.m_ctx.lineWidth = 2;
	this.m_ctx.beginPath();
	this.m_ctx.moveTo(this.m_iMiddleLinePos, 0);
	this.m_ctx.lineTo(this.m_iMiddleLinePos, this.m_iHeight);
	this.m_ctx.stroke();
}

/*************************************************
Function:		updateScalePos
Description:	更新刻度
Input:			无
Output:			无
return:			无
*************************************************/
TimeBar.prototype.updateScalePos = function()
{
	if (this.ScaleInfo.length == 0)
	{
		return;
	}
	// 以00:00移动的距离为准
	var seconds = (this.ScaleInfo[0].m_iHour - this.m_tCurrentMidTime.m_iHour) * 3600 + (this.ScaleInfo[0].m_iMinute - this.m_tCurrentMidTime.m_iMinute) * 60 + (this.ScaleInfo[0].m_iSecond - this.m_tCurrentMidTime.m_iSecond);
	var iPos0 = this.m_iMiddleLinePos + parseInt(parseFloat(seconds / (3600*this.m_fCellTime)) * this.m_iCellWidth);
	if (iPos0 < this.ScaleInfo[0].m_ixMin)
	{
		iPos0 = this.ScaleInfo[0].m_ixMax - (this.ScaleInfo[0].m_ixMin - iPos0);
	}
	else if (iPos0 > this.ScaleInfo[0].m_ixMax)
	{
		iPos0 = this.ScaleInfo[0].m_ixMin + (iPos0 - this.ScaleInfo[0].m_ixMax);
	}
	var iMoved = iPos0 - this.ScaleInfo[0].m_ix;
	//没有移动直接返回
	if(iMoved == 0)
	{
		return;
	}

	// 更新所有的刻度
	for (var i = 0; i < this.ScaleInfoNum; i++)
	{
		var iScalePos = this.ScaleInfo[i].m_ix + iMoved;
		// 设置刻度位置范围
		this.ScaleInfo[i].setPosRange(this.m_iFileListStartPos, this.m_iFileListStartPos + parseInt(this.m_iCellWidth * this.ScaleInfoNum));
		this.ScaleInfo[i].setPos(iScalePos, this.m_iTimeRectHeight);
	}
}

/*************************************************
Function:		updateFileListPos
Description:	更新文件
Input:			无
Output:			无
return:			无
*************************************************/
TimeBar.prototype.updateFileListPos= function()
{
	var iFileLength = this.FileInfoSet[this.m_iSelWnd].length;
	if(iFileLength == 0)
	{
		return;
	}
	var tStartTime = this.FileInfoSet[this.m_iSelWnd][0].m_tStartTime;
	var seconds = parseInt((tStartTime.m_iMilliseconds - this.m_tCurrentMidTime.m_iMilliseconds)/1000);
	var iFile0Pos = this.m_iMiddleLinePos + parseInt(parseFloat(seconds / (3600*this.m_fCellTime)) * this.m_iCellWidth);
	var iMoved = iFile0Pos - this.FileInfoSet[this.m_iSelWnd][0].m_iX;
	//没有移动直接返回
	if(iMoved == 0)
	{
		return;
	}

	// 更新所有
	for (i = 0; i < iFileLength; i++)
	{
		var iX = this.FileInfoSet[this.m_iSelWnd][i].m_iX + iMoved;
		var iY = this.FileInfoSet[this.m_iSelWnd][i].m_iY;
		var iWidth = this.FileInfoSet[this.m_iSelWnd][i].m_iWidth;
		var iHeight = this.FileInfoSet[this.m_iSelWnd][i].m_iHeight;
		this.FileInfoSet[this.m_iSelWnd][i].setPos(iX, iY, iWidth, iHeight);
	}
}

/*************************************************
Function:		resize
Description:	重置大小
Input:			iWidth宽度, iHeight高度
Output:			无
return:			无
*************************************************/
TimeBar.prototype.resize = function(iWidth, iHeight)
{
	this.m_canvas.height = iHeight;
	this.m_canvas.width = iWidth;
	this.m_iHeight = iHeight;
	this.m_iWidth = iWidth;
	this.m_iTimeRectHeight = parseInt(this.m_iHeight * 4 / 7);
	this.m_iFileRectHeight = this.m_iHeight - this.m_iTimeRectHeight - this.m_iBlankHeight * 2;
	this.m_iMiddleLinePos = parseInt((this.m_iFileListStartPos + this.m_iWidth) / 2);
	this.m_iCellWidth = Math.floor((this.m_iWidth - this.m_iFileListStartPos)/this.ScaleInfoDisNum);
	this.m_iCellMilliseconds = parseInt((3600 * this.m_fCellTime * 1000)/this.m_iCellWidth, 10);
	
	//初始化时间刻度信息
	for (i = 0; i < this.ScaleInfoNum; i++)
	{
		// 计算与中轴线的时间差（只计算时分秒）
		var seconds = (this.ScaleInfo[i].m_iHour - this.m_tCurrentMidTime.m_iHour) * 3600 + (this.ScaleInfo[i].m_iMinute - this.m_tCurrentMidTime.m_iMinute) * 60 + (this.ScaleInfo[i].m_iSecond - this.m_tCurrentMidTime.m_iSecond);
		var iScalePos = this.m_iMiddleLinePos + parseInt(parseFloat(seconds / (3600*this.m_fCellTime)) * this.m_iCellWidth);
		// 设置刻度位置范围
		this.ScaleInfo[i].setPosRange(this.m_iFileListStartPos, this.m_iFileListStartPos + parseInt(this.m_iCellWidth * this.ScaleInfoNum));
		this.ScaleInfo[i].setPos(iScalePos, this.m_iTimeRectHeight);
	}
	
	//初始化文件列表信息
	for (i = 0; i < this.FileInfoSet[this.m_iSelWnd].length; i++)
	{
		var FileInfoSetSel = this.FileInfoSet[this.m_iSelWnd][i];
		var iXLeftSeconds = parseInt((FileInfoSetSel.m_tStartTime.m_iMilliseconds - this.m_tCurrentMidTime.m_iMilliseconds)/1000);
		var iFilePosLeft = this.m_iMiddleLinePos + parseInt(parseFloat(iXLeftSeconds / (3600*this.m_fCellTime)) * this.m_iCellWidth);
		
		/*var iXRightSeconds = (tStopTime.m_iHour - this.m_tCurrentMidTime.m_iHour) * 3600 + (tStopTime.m_iMinute - this.m_tCurrentMidTime.m_iMinute) * 60 + (tStopTime.m_iSecond - this.m_tCurrentMidTime.m_iSecond);*/
		var iXRightSeconds = parseInt((FileInfoSetSel.m_tStopTime.m_iMilliseconds - this.m_tCurrentMidTime.m_iMilliseconds)/1000);
		
		var iFilePosRight = this.m_iMiddleLinePos + parseInt(parseFloat(iXRightSeconds / (3600*this.m_fCellTime)) * this.m_iCellWidth);
		if((iFilePosRight - iFilePosLeft) < this.m_iMinFileWidth)
		{
			iFilePosRight = iFilePosLeft + this.m_iMinFileWidth;
		}
		
		FileInfoSetSel.setPos(iFilePosLeft, this.m_iTimeRectHeight + parseInt(this.m_iBlankHeight/2), iFilePosRight - iFilePosLeft, this.m_iFileRectHeight + 2);
	}
	
	this.repaint();
}

/*************************************************
Function:		SetSpantype
Description:	设置时间条的显示样式
Input:			无
Output:			无
return:			无
*************************************************/
TimeBar.prototype.SetSpantype = function(iSpanType)
{
	switch(iSpanType)
	{
		case 6://每2小时一格
			this.ScaleInfoDisNum = 12;
			this.m_fCellTime = parseFloat(2.0);
			break;
		case 7://每小时一格
			this.ScaleInfoDisNum = 12;
			this.m_fCellTime = parseFloat(1.0);
			break;
		case 8://每半小时一格
			this.ScaleInfoDisNum = 12;
			this.m_fCellTime = parseFloat(0.5);
			break;
		case 9://每半小时一格
			this.ScaleInfoDisNum = 8;
			this.m_fCellTime = parseFloat(0.5);
			break;
		case 10://每１０分钟一格
			this.ScaleInfoDisNum = 12;
			this.m_fCellTime = parseFloat(1/6);
			break;
		case 11://每５分钟一格
			this.ScaleInfoDisNum = 12;
			this.m_fCellTime = parseFloat(1/12);
			break;
		case 12://每５分钟一格
			this.ScaleInfoDisNum = 6;
			this.m_fCellTime = parseFloat(1/12);
			break;
		default:
			this.ScaleInfoDisNum = 12;
			this.m_fCellTime = parseFloat(1.0);
			return;
	}
	this.ScaleInfoNum = parseInt(24/this.m_fCellTime, 10);
	this.m_iCellWidth = Math.floor((this.m_iWidth - this.m_iFileListStartPos)/this.ScaleInfoDisNum);
	this.m_iCellMilliseconds = parseInt((3600 * this.m_fCellTime * 1000)/this.m_iCellWidth, 10);
	
	//初始化刻度
	this.ScaleInfo.length = 0;
	for(var i = 0; i < this.ScaleInfoNum; i++) 
	{
		this.ScaleInfo.push(new ScaleInfo(0, 0, parseInt(i * 3600 * this.m_fCellTime)));
	}
	
	//初始化时间刻度信息
	for (i = 0; i < this.ScaleInfoNum; i++)
	{
		// 计算与中轴线的时间差（只计算时分秒）
		var seconds = (this.ScaleInfo[i].m_iHour - this.m_tCurrentMidTime.m_iHour) * 3600 + (this.ScaleInfo[i].m_iMinute - this.m_tCurrentMidTime.m_iMinute) * 60 + (this.ScaleInfo[i].m_iSecond - this.m_tCurrentMidTime.m_iSecond);
		var iScalePos = this.m_iMiddleLinePos + parseInt(parseFloat(seconds / (3600*this.m_fCellTime)) * this.m_iCellWidth);
		// 设置刻度位置范围
		this.ScaleInfo[i].setPosRange(this.m_iFileListStartPos, this.m_iFileListStartPos + parseInt(this.m_iCellWidth * this.ScaleInfoNum));
		this.ScaleInfo[i].setPos(iScalePos, this.m_iTimeRectHeight);
	}
	
	//初始化文件列表信息
	for (i = 0; i < this.FileInfoSet[this.m_iSelWnd].length; i++)
	{
		var FileInfoSetSel = this.FileInfoSet[this.m_iSelWnd][i];
		var iXLeftSeconds = parseInt((FileInfoSetSel.m_tStartTime.m_iMilliseconds - this.m_tCurrentMidTime.m_iMilliseconds)/1000);
		var iFilePosLeft = this.m_iMiddleLinePos + parseInt(parseFloat(iXLeftSeconds / (3600*this.m_fCellTime)) * this.m_iCellWidth);
		
		/*var iXRightSeconds = (tStopTime.m_iHour - this.m_tCurrentMidTime.m_iHour) * 3600 + (tStopTime.m_iMinute - this.m_tCurrentMidTime.m_iMinute) * 60 + (tStopTime.m_iSecond - this.m_tCurrentMidTime.m_iSecond);*/
		var iXRightSeconds = parseInt((FileInfoSetSel.m_tStopTime.m_iMilliseconds - this.m_tCurrentMidTime.m_iMilliseconds)/1000);
		
		var iFilePosRight = this.m_iMiddleLinePos + parseInt(parseFloat(iXRightSeconds / (3600*this.m_fCellTime)) * this.m_iCellWidth);
		if((iFilePosRight - iFilePosLeft) < this.m_iMinFileWidth)
		{
			iFilePosRight = iFilePosLeft + this.m_iMinFileWidth;
		}
		
		FileInfoSetSel.setPos(iFilePosLeft, this.m_iTimeRectHeight + parseInt(this.m_iBlankHeight/2), iFilePosRight - iFilePosLeft, this.m_iFileRectHeight + 2);
	}
	
	this.repaint();
}
/*************************************************
Function:		addFile
Description:	添加文件
Input:			StartTime 开始时间, StopTime 结束时间, iType 类型, iWndNum 默认当前窗口 添加到某个窗口
Output:			无
return:			无
*************************************************/
TimeBar.prototype.addFile = function(StartTime, StopTime, iType/*, iWndNum*/)
{
	var tStartTime = new Time();
	var tStopTime = new Time();
	tStartTime.parseTime(StartTime);
	tStopTime.parseTime(StopTime);
	var fileColor;
	switch (iType)
	{
		case 1:
			fileColor = this.scheFileColor;
			break;
		case 2:
			fileColor = this.alarmFileColor;
			break;
		case 3:
			fileColor = this.cmdFileColor;
			break;
		case 4:
			fileColor = this.manualFileColor;
			break;
		default:
			fileColor = this.defaultFileColor;
			break;
	}
	/*var iXLeftSeconds = (tStartTime.m_iHour - this.m_tCurrentMidTime.m_iHour) * 3600 + (tStartTime.m_iMinute - this.m_tCurrentMidTime.m_iMinute) * 60 + (tStartTime.m_iSecond - this.m_tCurrentMidTime.m_iSecond);*/
	var iXLeftSeconds = parseInt((tStartTime.m_iMilliseconds - this.m_tCurrentMidTime.m_iMilliseconds)/1000);
	var iFilePosLeft = this.m_iMiddleLinePos + parseInt(parseFloat(iXLeftSeconds / (3600*this.m_fCellTime)) * this.m_iCellWidth);
	
	/*var iXRightSeconds = (tStopTime.m_iHour - this.m_tCurrentMidTime.m_iHour) * 3600 + (tStopTime.m_iMinute - this.m_tCurrentMidTime.m_iMinute) * 60 + (tStopTime.m_iSecond - this.m_tCurrentMidTime.m_iSecond);*/
	var iXRightSeconds = parseInt((tStopTime.m_iMilliseconds - this.m_tCurrentMidTime.m_iMilliseconds)/1000);
	
	var iFilePosRight = this.m_iMiddleLinePos + parseInt(parseFloat(iXRightSeconds / (3600*this.m_fCellTime)) * this.m_iCellWidth);
	
	var fileInfo = new FileInfo(iFilePosLeft, this.m_iTimeRectHeight + parseInt(this.m_iBlankHeight/2), iFilePosRight - iFilePosLeft, this.m_iFileRectHeight + 2, iType, fileColor, tStartTime, tStopTime);
	fileInfo.setPosRange(this.m_iFileListStartPos, this.m_iFileListStartPos + parseInt(this.m_iCellWidth * this.ScaleInfoNum));
	if(arguments.length >= 4)
	{
		this.FileInfoSet[arguments[3]].push(fileInfo);
	}
	else
	{
		this.FileInfoSet[this.m_iSelWnd].push(fileInfo);
	}
}

/*************************************************
Function:		clearWndFileList
Description:	清空某个窗口的文件信息
Input:			iWndNum 窗口号 0-15 默认当前选中窗口
Output:			无
return:			无
*************************************************/
TimeBar.prototype.clearWndFileList = function()/*iWndNum*/
{
	var iWndParam;
	if(arguments.length == 0)
	{
		iWndParam = this.m_iSelWnd;
	}
	else
	{
		iWndParam = arguments[0];
	}
	if(iWndParam < 0)
	{
		iWndParam = 0;
	}
	if(iWndParam >= 16)
	{
		iWndParam = 15
	}
	this.FileInfoSet[iWndParam].length = 0;
}

/*************************************************
Function:		setMidLineTime
Description:	设置中轴线时间
Input:			szTime yyyy-MM-dd HH:mm:ss
Output:			无
return:			无
*************************************************/
TimeBar.prototype.setMidLineTime = function(szTime)
{
	var tCurTime = new Time();
	tCurTime.parseTime(szTime);
	this.m_tCurrentMidTime.setTimeByMis(tCurTime.m_iMilliseconds);
	/*this.updateScalePos();
	this.updateFileListPos();*/
	this.repaint();
}

/*************************************************
Function:		setMouseUpCallback
Description:	设置鼠标弹起回调函数
Input:			func 回调函数 function(tStartTime, tStopTime)
Output:			无
return:			无
*************************************************/
TimeBar.prototype.setMouseUpCallback = function(callbackFunc)
{
	this.mouseUpCallbackFunc = callbackFunc;
}