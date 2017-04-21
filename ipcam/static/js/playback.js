var m_iWndNum =	0;
var m_szBrowser	= navigator.appName; //获取浏览器名称

var m_szXmlStr = "";			    //xml保存

var m_iWndSpeed = 1;			    //当前的播放速度

var m_bWndSearched = false;		    //窗口是否搜索过
var m_tWndMidTime = "";		        //窗口的时间轴时间
var tTimeBar = null;                //时间条对象
var m_dtSearchDate = "";		    //当前搜索的日期
var m_dtCalendarDate = "";		    //当前日历的日期

var m_iOSDTimer = 0;				//获取OSD定时器
var m_iOSDLast = 0;
var m_iOSDCount = 0;

var g_iSpanType = 7;

var m_channelCamid = new Array();

/*************************************************
Function:		InitPlayback
Description:	初始化回放界面
Input:			无
Output:			无
return:			无
*************************************************/
function InitPlayback()
{
/*
	DIY streaming:
        m_szUserPwdValue = "530309d585cb310609017c03 1394939030 TEST";
        
        live555 streaming:
        m_1stStream = /530309d585cb310609017c03_1394939030
*/

	m_iWndSpeed = 1;
	m_iWndNum = 4;

	for (var i=0; i<m_iWndNum; i++){
		m_szUserPwdValue[i] = "-1";
		m_playbackStream[i] = "/";
		m_szHostName[i]=location.hostname;
		m_channelCamid[i]="-1";
	}
	g_szHttpPort = "2500";
	m_lRtspPort = "2500";
	m_iStreamType=3; 
	
	HWP = new Plugin(m_iWndNum, m_szHostName, g_szHttpPort,	m_lRtspPort);
	
	var date =  new Time();
	m_dtSearchDate = date.getStringTime().split(" ")[0];
	m_dtCalendarDate = m_dtSearchDate;

	WdatePicker({minDate:'2000-01-01 00:00:00',
		     maxDate:'2024-12-31 23:59:59',
		     eCont:'div1',
		     onpicked:function(dp){m_dtCalendarDate = dp.cal.getDateStr();},
		     lang: 'zh-cn',
		     startDate:m_dtCalendarDate});

	if(!$.browser.msie){
		var canvas = document.getElementById("timebar");
		if (canvas.getContext){
			tTimeBar = new TimeBar(canvas, 806, 60);
			tTimeBar.setMouseUpCallback(timeBarMouseUp);
		} 
		else {
			$("#timebar").html('');
		}
	}
	else{
		$("#timebar").width(806);
		$("#timebar").height(60);
		tTimeBar = document.getElementById("timebar");
	}
	if(!checkPlugin('2', null, Math.ceil(Math.sqrt(m_iWndNum)))){
		$('#main_plugin').attr("style", "text-align:center;line-height:498px; background-color:#343434;");
		if($.browser.msie){
		    $("#playbackbar").hide();
		}
		return;
	}
	else
	{
		$("#dvTimebarCtrl").show();
		$("#frTimebarCtrl").show();		
	}

	m_PreviewOCX = document.getElementById("PreviewActiveX");
	
	if(!CompareFileVersion()){
		UpdateTips();
	}
	//initVolumeSlider();	
	initMouseHover();
}
/*************************************************
Function:		showTips
Description:	显示提示语
Input:			title:标题
				strTips: 提示语
Output:			无
return:			无
*************************************************/
var g_iShowTipsTimer;
function showTips(title, strTips)
{
	$('#laPlaybackTips').html(strTips);
	$('#divPlaybackTips').show();
	clearTimeout(g_iShowTipsTimer);
	g_iShowTipsTimer = setTimeout(function()
			   {
				   $('#laPlaybackTips').html('');
				   $('#divPlaybackTips').hide(); 
			   },  5000);
}
/*************************************************
Function:		SearchRecordFile
Description:	搜索录像文件
Input:			iType:0拖动时间轴跨天搜索，1非首次搜索，2点击搜索按钮搜索
				dtStartTime: 开始时间(可省略)
				dtStopTime:  结束时间(可省略)
Output:			无
return:			无
*************************************************/
function SearchRecordFile(iType, szStartTime, szStopTime)
{
	showTips('', '正在搜索录像 ...');
	
	if (m_channelCamid[0]=="-1"){
		showTips('', '请先选择第1路的相机');
		return;
	}
	
	if(iType == 2)
	{
		if(HWP.wnds[0].isPlaying)		//如果当前窗口正在回放
		{
			showTips('', '请先停止回放');
			return;
		}
		if(arguments.length > 1)
		{
			m_dtSearchDate = szStartTime.split(" ")[0];
			m_tWndMidTime = szStartTime;
		}
		else
		{
			m_dtSearchDate = m_dtCalendarDate;
			m_tWndMidTime = m_dtSearchDate + " 00:00:00";
		}
		
		tTimeBar.setMidLineTime(m_tWndMidTime);		
	}

	if(iType != 1)	//搜索前先清空时间条
	{
		m_szXmlStr = '';
		
		tTimeBar.m_iSelWnd = 0;
		tTimeBar.clearWndFileList(0);
		tTimeBar.repaint();

		var dtStartTime = "";
		var dtStopTime = "";
		if(arguments.length > 1)
		{
			dtStartTime = szStartTime.replace(' ', 'T') + 'Z';
			dtStopTime = szStopTime.replace(' ', 'T') + 'Z';
		}
		else
		{
			dtStartTime = (m_dtSearchDate+" 00:00:00").replace(' ', 'T') + 'Z';
			dtStopTime = (m_dtSearchDate+" 23:59:59").replace(' ', 'T') + 'Z';
		}		
	}


	$.ajax({
		type: "GET",
		url: "/search",
		async: true,
		timeout: 15000,
		//processData: false,
		data: {camid:m_channelCamid[0],st:dtStartTime, et:dtStopTime},
		beforeSend: function(xhr) {
			xhr.setRequestHeader("If-Modified-Since", "0");
			xhr.setRequestHeader("Authorization", "Basic " + m_szUserPwdValue);
		},
		complete: function(xhr, textStatus)
		{
			if(xhr.status == 403)
			{
				showTips('', '无操作权限');
			}
			else if(xhr.status==200)
			{
				var retStr = xhr.responseText;
				
				if (retStr.split("|")[0]>0){
					m_bWndSearched = true;
					m_szXmlStr = retStr.split("|");
					if(Number(m_szXmlStr[0]) > 0)
					{
						AddFileToTimeBar(m_szXmlStr, 0);
					}
					showTips('', '搜索到 '+m_szXmlStr[0]+' 段录像');	
				}
				else{
					showTips('', '没有搜索到录像');
				}
			}
			else
			{
				m_bWndSearched = false;
				m_szXmlStr = '';
				showTips('', '搜索录像失败！');
				return ;
			}
		}
	});
}

/*************************************************
Function:		AddFileToTimeBar
Description:	添加文件到时间条
Input:			xmlDoc:文件信息
Output:			无
return:			无
*************************************************/
function AddFileToTimeBar(time2)
{
	var timeNum=Number(time2[0]);
	
	if (timeNum==0) return;
	
	if($.browser.msie){
		xmlStr="<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
		  +"<CMSearchResult>"
		  +"<numOfMatches>2</numOfMatches>"
		  +"<matchList>";
		for(var i=0; i<timeNum; i++){
			xmlStr=xmlStr+'<searchMatchItem><timeSpan><startTime>'
				+time2[i*2+1]+'</startTime><endTime>'
				+time2[i*2+2]+'</endTime></timeSpan></searchMatchItem>';
		}		
		xmlStr+='</matchList></CMSearchResult>';
		tTimeBar.AddFileList(xmlStr,0);
	}
	else{
		for(var i = 0; i < timeNum; i++)
		{
			tTimeBar.addFile((time2[i*2+1].replace('T', ' ')).replace('Z', ''), 
					 (time2[i*2+2].replace('T', ' ')).replace('Z', ''), 1);
		}
	}
	
	tTimeBar.setMidLineTime((time2[1].replace('T', ' ')).replace('Z', ''));
	tTimeBar.repaint();
	
	showTips('', '搜索完成');
}
/*************************************************
Function:		GetSelectWndInfo
Description:	获取选中窗口信息
Input:			SelectWndInfo:窗口信息xml
Output:			无
return:			无
*************************************************/
function GetSelectWndInfo(SelectWndInfo)
{
	return;
}
/*************************************************
Function:		PluginEventHandler
Description:	回放事件响应
Input:			iEventType 事件类型, iParam1 参数1, iParam2 保留
Output:			无
return:			无
*************************************************/
function PluginEventHandler(iEventType, iParam1, iParam2)
{
	if(0 == iEventType)
	{
		GetOSDTime(iParam1);
		if(0 == iParam1)
		{
			StopPlayBack();
		}
		showTips('', '回放异常结束');
	}
	else if(2 == iEventType)
	{
		if(0 == iParam1)
		{
			StopPlayBack();
		}
		showTips('', '回放结束');
	}
}
/*************************************************
Function:		timeBarMouseUp
Description:	鼠标离开时间条时，跨天了就继续搜索
Input:			tMidTime 中轴线时间
Output:			无
return:			无
*************************************************/
function timeBarMouseUp(tMidTime)
{
	 var bPlay = HWP.wnds[0].isPlaying;
	 StopPlayBack();

	 var date;
	 if(arguments.length >= 1)
	 {
		 date = tMidTime;
	 }
	 else
	 {
		 date = tTimeBar.m_tCurrentMidTime.getStringTime();
	 }
	 m_tWndMidTime = date;	
	 var szStartTime = m_dtSearchDate+" 00:00:00";
	 var szStopTime = m_dtSearchDate+" 23:59:59";
	 
	 if(date > szStopTime || date < szStartTime)
	 {
		 m_dtSearchDate = date.split(" ")[0];
		 if(m_bWndSearched)
		 {
			 SearchRecordFile(0);
		 }
	 }
	 if(bPlay)
	 {
		 StartPlayBack();
	 }
}

/**********************************
Function:		StartPlayBack
Description:	开始回放、暂停回放
Input:			
Output:			无
return:			无
***********************************/
function StartPlayBack()
{	
	if (!HWP.wnds[0].isPlaying)	//没有回放
	{
		var date1;
		if(navigator.appName == "Microsoft Internet Explorer")
		{
			date1 =  new Date(Date.parse(tTimeBar.GetPlayBackTime().replace(/\-/g,'/')));
		}
		else
		{
			date1 =  new Date(Date.parse(tTimeBar.m_tCurrentMidTime.getStringTime().replace(/\-/g,'/')));
		}
		var date2 =  new Date(Date.parse((m_dtSearchDate+" 23:59:59").replace(/\-/g,'/')));		
		if(date1.getTime() >= date2.getTime())
		{
			showTips('', '请先搜索文件');
			return;
		}
		
		var szStartTime = date1.Format("yyyyMMddThhmmssZ"); 		//当前时间轴时间
		var szStopTime = date2.Format("yyyyMMddThhmmssZ"); 		//时间轴后一天的23:59:59
		
		if (!m_bWndSearched){
			showTips('', '请先搜索录像');
			return;
		}
		
		if (activeCamNum()>1) HWP.ArrangeWindow(2);
		else HWP.ArrangeWindow(1);
		
		var play_ret=0;
		$.each(m_channelCamid, function(i) {
			if (m_channelCamid[i]!='-1'){
				var tick_str=String(date1.getTime()/1000);
				m_szUserPwdValue[i]=m_channelCamid[i]+" "+tick_str+" TEST";
				//m_szUserPwdValue[i]="F8KAM"
				m_playbackStream[i]="/"+m_channelCamid[i]+"_"+tick_str+"_"+String(m_iWndSpeed);
				if (tick_str<"1405699200"){ // mile stone
					m_lRtspPort = "2501";
				}
				else{
					m_lRtspPort = "2500";
				}
				if (HWP.Play(i)!=0) play_ret=-1;
			}
		});
		if (play_ret==0) {
			$("#play").removeClass().addClass("pause").attr("title",'暂停');
			$("#nowStatues").html('回放中');
			GetOSDTime(0);
			$("#capture").removeClass().addClass("capture");
			$("#stop").removeClass().addClass("stop");
			$("#SlowlyForward").removeClass().addClass("slowlyforward");
			$("#FastForward").removeClass().addClass("fastforward");
			disableSelect(true);
		} else {
			showTips("","回放失败");
		}

	} else {//回放中
		if (!HWP.wnds[0].isPause) {//当前为回放状态
			if (0 == HWP.Pause()) {
				clearTimeout(m_iOSDTimer);
				m_iOSDTimer = 0;
				$("#play").removeClass().addClass("play").attr("title",'恢复');
				$("#nowStatues").html('已暂停');
				
				//记住当前窗口的时间轴时间
				if(navigator.appName == "Microsoft Internet Explorer") {
					m_tWndMidTime = tTimeBar.GetPlayBackTime();		
				} else {
					m_tWndMidTime = tTimeBar.m_tCurrentMidTime.getStringTime();
				}
				$("#SlowlyForward").removeClass().addClass("slowlyforwarddisable");
				$("#FastForward").removeClass().addClass("fastforwarddisable");
			} else {
				showTips('', '暂停失败');
			}
		} else {
			if (0 == HWP.Resume()) {
				GetOSDTime(0);
				$("#play").removeClass().addClass("pause").attr("title",'暂停');
				$("#nowStatues").html('回放中');
				$("#SlowlyForward").removeClass().addClass("slowlyforward");
				$("#FastForward").removeClass().addClass("fastforward");
			} else {
				showTips('', '恢复回放失败');
			}
		}
	}
}
/**********************************
Function:		StopPlayBack
Description:	停止回放
Input:			
Output:			无
return:			无	
***********************************/
function StopPlayBack2()
{
	m_iWndSpeed = 1;
	StopPlayBack();
	$("#laCurrentSpeed").html(m_iWndSpeed+' 倍速');
}
function StopPlayBack()
{
	if(HWP.wnds[0].isPlaying)
	{
		if(0 == HWP.Stop())
		{
			$("#play").removeClass().addClass("play").attr("title",'播放');
			clearTimeout(m_iOSDTimer);		//关闭OSD获取的定时器
			m_iOSDTimer = 0;

			//记住当前窗口的时间轴时间
			if(navigator.appName == "Microsoft Internet Explorer")
			{
				m_tWndMidTime = tTimeBar.GetPlayBackTime();		
			}
			else
			{
				m_tWndMidTime = tTimeBar.m_tCurrentMidTime.getStringTime();
			}
			$("#nowStatues").html("停止回放");
			$("#capture").removeClass().addClass("capturedisable");
			$("#stop").removeClass().addClass("stopdisable");
			$("#SlowlyForward").removeClass().addClass("slowlyforwarddisable");
			$("#FastForward").removeClass().addClass("fastforwarddisable");
			disableSelect(false);
		}
		else
		{
			showTips('',  '停止回放失败');
		}
	}
}
/**********************************
Function:		PlayBackSlowly
Description:	慢放
Input:			
Output:			无
return:			无
***********************************/
function PlayBackSlowly()
{
	if(HWP.wnds[0].isPlaying && m_iWndSpeed > 1)
	{
		StopPlayBack();
		m_iWndSpeed = 1/2 * m_iWndSpeed;
		$("#laCurrentSpeed").html(m_iWndSpeed+' 倍速');
		StartPlayBack();
	}
}
/**********************************
Function:		PlayBackFast
Description:	快放
Input:			
Output:			无
return:			无
***********************************/
function PlayBackFast()
{
	if(HWP.wnds[0].isPlaying && m_iWndSpeed < 8)
	{
		StopPlayBack();
		m_iWndSpeed = 2 * m_iWndSpeed;
		$("#laCurrentSpeed").html(m_iWndSpeed+' 倍速');
		StartPlayBack();
	}
}
/**********************************
Function:		GetOSDTime
Description:	获取OSD时间
Input:			iWndNum:窗口号
Output:			无
return:			无
***********************************/
function GetOSDTime(iWndNum)
{
	if(!HWP.wnds[0].isPlaying)
	{
		return;
	}

	var iTime = m_PreviewOCX.HWP_GetOSDTime(0);
	if(iTime <= 8 * 3600 * 1000)
	{
		if(m_iOSDTimer != 0)
		{
			clearTimeout(m_iOSDTimer);
			m_iOSDTimer = 0;
		}
		m_iOSDTimer = setTimeout("GetOSDTime(0)", 100);
		return;
	}
	if(navigator.appName == "Microsoft Internet Explorer")
	{
		if(tTimeBar.GetMouseDown())
		{
			if(m_iOSDTimer != 0)
			{
				clearTimeout(m_iOSDTimer);
				m_iOSDTimer = 0;
			}
			m_iOSDTimer = setTimeout("GetOSDTime(0)", 1000);
			return;
		}
	}
	else
	{
		if(tTimeBar.m_bMOuseDown)
		{
			if(m_iOSDTimer != 0)
			{
				clearTimeout(m_iOSDTimer);
				m_iOSDTimer = 0;
			}
			m_iOSDTimer = setTimeout("GetOSDTime(0)", 1000);
			return;
		}
	}
	var date = new Date(iTime * 1000);

	m_tWndMidTime = date.Format("yyyy-MM-dd hh:mm:ss");

	tTimeBar.setMidLineTime(m_tWndMidTime);
	
	if(m_iOSDTimer != 0)
	{
		clearTimeout(m_iOSDTimer);
		m_iOSDTimer = 0;
	}
	m_iOSDTimer = setTimeout("GetOSDTime(0)", 1000);
}
/**********************************
Function:		CapturePicture
Description:	抓图
Input:			无
Output:			无
return:			无
***********************************/
function CapturePicture()
{
	if(HWP.wnds[0].isPlaying)
	{
		var time = new Date();
		var szFileName = "";

		szFileName = time.Format("yyyy-MM-dd_hh.mm.ss.S");
		
		var iRes = m_PreviewOCX.HWP_CapturePicture(0, szFileName);
		if(iRes == 0)
		{
			showTips("",'抓图成功');
		}
		else if(iRes == -1)
		{
			var iError = m_PreviewOCX.HWP_GetLastError();
			if(10 == iError || 9 == iError)
			{
				showTips('', '创建抓图文件失败。');
			}
			else
			{
				showTips('', '抓图失败！');
			}
		}
		else if(-2 == iRes)
		{
			showTips('', 'FreeSpaceTips');
		}
		else if(-3 == iRes)
		{
			showTips('', 'jsCreateFileFailed');
		}
		else
		{
		}
	}
}
/*************************************************
Function:		GoTime
Description:	定位
Input:			无
Output:			无
return:			无
*************************************************/
function GoTime()
{
	var time = m_dtCalendarDate + " " + $('#time_shi').val()+":"+$('#time_fen').val()+":"+$('#time_miao').val();
	tTimeBar.setMidLineTime(time);
	timeBarMouseUp();
}
/*************************************************
Function:		expandTimebar
Description:	扩大时间条
Input:			无			
Output:			无
return:			无				
*************************************************/
function expandTimebar()
{
	g_iSpanType++;
	if(g_iSpanType > 12)
	{
		g_iSpanType = 12;
		return;
	}
	try
	{
		tTimeBar.SetSpantype(g_iSpanType);
	}
	catch(e)
	{
	}
}
/*************************************************
Function:		narrowTimebar
Description:	缩短时间条
Input:			无			
Output:			无
return:			无				
*************************************************/
function narrowTimebar()
{
	g_iSpanType--;
	if(g_iSpanType < 6)
	{
		g_iSpanType = 6;
		return;
	}
	try
	{
		tTimeBar.SetSpantype(g_iSpanType);
	}
	catch(e)
	{
	}
}

/*************************************************
Function:		initMouseHover
Description:	初始化鼠标悬浮样式
Input:			无			
Output:			无
return:			无				
*************************************************/
function initMouseHover()
{
	$(".buttonmouseout").each(function()
	{
		$(this).hover
		(
			function () 
			{
			    if($(this).children().hasClass("capturedisable") || $(this).children().hasClass("recorddisable") || $(this).children().hasClass("stopdisable") || $(this).children().hasClass("slowlyforwarddisable") || $(this).children().hasClass("fastforwarddisable") || $(this).children().hasClass("singleframedisable"))
				{
					
				}
				else
				{
				    $(this).removeClass().addClass("buttonmouseover");
				}
			},
			function () 
			{
		$(this).removeClass().addClass("buttonmouseout");
			}
		);
	});
	$(".volumemouseout").each(function()
	{
		$(this).hover
		(
			function () 
			{
			    if($(this).children().hasClass("sounddisable"))
				{
					
				}
				else
				{
				    $(this).removeClass().addClass("volumemouseover");
				}
			},
			function () 
			{
		$(this).removeClass().addClass("volumemouseout");
			}
		);
	});		
}

/*************************************************
Function:		size4to3
Description:	插件按照4:3显示
Input:			无			
Output:			无
return:			无				
*************************************************/
function size16to9() {
	$("#content").width(938);
	$("#dvChangeSize").width(938);
	$("#main_plugin").width(938).height(527);
	$("#toolbar").width(938);
}
/*************************************************
Function:		changeCamid
Description:	修改相机选择
Input:			无			
Output:			无
return:			无				
*************************************************/
function changeCamid(chan) {
	chan_id="#cam_"+chan;
	m_channelCamid[chan]=$(chan_id)[0].value;
}
/*************************************************
Function:		disableSelect
Description:	屏蔽选择相机的select
Input:			无			
Output:			无
return:			无				
*************************************************/
function disableSelect(boo) {
	if (boo){
		$("#cam_0").attr("disabled","disabled");
		$("#cam_1").attr("disabled","disabled");
		$("#cam_2").attr("disabled","disabled");
		$("#cam_3").attr("disabled","disabled");
	}
	else{
		$("#cam_0").removeAttr("disabled");
		$("#cam_1").removeAttr("disabled");
		$("#cam_2").removeAttr("disabled");
		$("#cam_3").removeAttr("disabled");
	}
}
/*************************************************
Function:		activeCamNum
Description:	同时回放的相机数
Input:			无			
Output:			无
return:			无				
*************************************************/
function activeCamNum(){
	var ret=0;
	$.each(m_channelCamid, function(i) {	
		if (m_channelCamid[i]!='-1') ret++;
	});
	return ret;
}
/*************************************************
Function:		parseTime
Description:	通过时间字符串转换成时间戳
Input:			szTime 时间 yyyy-MM-dd HH:mm:ss
Output:			无
return:			无
*************************************************/
function parseTime(szTime){
	var aDate = szTime.split(' ')[0].split('-');
	var aTime = szTime.split(' ')[1].split(':');
	
	var tTime = new Date();
	tTime.setFullYear(parseInt(aDate[0],10));
	tTime.setMonth(parseInt(aDate[1],10) - 1, parseInt(aDate[2],10));
	
	tTime.setHours(parseInt(aTime[0],10));
	tTime.setMinutes(parseInt(aTime[1],10));
	tTime.setSeconds(parseInt(aTime[2],10));
	
	return tTime.getTime()/1000;
}
