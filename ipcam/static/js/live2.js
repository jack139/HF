var m_bIsDiskFreeSpaceEnough = true;
var m_szBrowser = navigator.appName; //获取浏览器名称
var m_iWndNum = 0;
var m_iPtzMode = 1;//0为显示,1为隐藏0;
var m_iOperateMode = 0;	//弹出编辑路径窗口后的操作	0 - 添加 1 - 修改
var m_oOperated	= null;	//被操作修改值的对象
var m_iProtocolType = 0;	//取流方式，默认为RTSvar m_bLight = false;	     //ptz灯光状态
var g_lxdPreview = null; // Preview.xml
var m_iDelayTimer = 0;

/*************************************************
Function:		InitPreview
Description:	初始化预览界面
Input:			无
Output:			无
return:			无
*************************************************/
function InitLive2() {		
	HWP = new Plugin(m_iWndNum, m_szHostName, g_szHttpPort,	m_lRtspPort);
	
	if(!checkPlugin('2', null, Math.ceil(Math.sqrt(m_iWndNum)))) {
		$('#main_plugin').attr("style",	"text-align:center;line-height:576px;background-color:#343434;");
		return;
	}
	m_PreviewOCX=document.getElementById("PreviewActiveX");
	//设置基本信息
	var szInfo = '<?xml version="1.0" encoding="utf-8"?><Information><WebVersion><Type>ipc</Type><Version>3.1.2.120416</Version><Build>20120416</Build></WebVersion><PluginVersion><Version>3.0.3.5</Version><Build>20120416</Build></PluginVersion><PlayWndType>0</PlayWndType></Information>';
	try {
		m_PreviewOCX.HWP_SetSpecialInfo(szInfo,	0);
	} catch(e) {	
	}
	//比较插件版本信息
	if(!CompareFileVersion()) {
		UpdateTips();
	}
	var szPathInfo = '';
	try {
		szPathInfo = m_PreviewOCX.HWP_GetLocalConfig();
	} catch(e) {
		szPathInfo = m_PreviewOCX.GetLocalConfig();
	}
	var xmlDoc = parseXmlFromStr(szPathInfo);
	m_iProtocolType	= parseInt($(xmlDoc).find("ProtocolType").eq(0).text(),	10);
	
	//某些初始化延后执行
	if(m_iStreamType==0) size16to9();
	else size4to3();
	setTimeout("StartRealPlay()", 100);

	$("#fullscreen1").removeClass().addClass("fullscreen1over");
	$("#fullscreen2").removeClass().addClass("fullscreen2out");
	$("#substream").removeClass().addClass("substreamover");
	$("#mainstream").removeClass().addClass("mainstreamout");
}
/*************************************************
Function:		StartRealPlay
Description:	开始预览
Input:			iChannelNum：通道号
		iWndNum：窗口号  （默认当前选中窗口）
Output:			无
return:			无
*************************************************/
function DelayTime()
{
	if(!HWP.isPlaying()) {
		StartRealPlay0()
		m_iDelayTimer = setTimeout("DelayTime()", 1800000); // 30分钟后重启一次播放
	}else{
		StopRealPlay();
		setTimeout("DelayTime()", 100);
	}	
}

function StartRealPlay() {
	if(!HWP.isPlaying()) {
		setTimeout("DelayTime()", 100);
		$("#play").removeClass().addClass("stoprealplay");
	}else{
		clearTimeout(m_iDelayTimer);
		StopRealPlay();
		$("#play").removeClass().addClass("play");
	}
}

function StartRealPlay0() {
	var e1=0;
	var e2=0;

	for(var	i=0; i<m_iWndNum; i++){
		if(HWP.Play(i)!=0){
			var iError = m_PreviewOCX.HWP_GetLastError();
			if(403 == iError) {
				//alert('第'+(i+1)+'路，操作无权限！');
				e1++;
			} else {
				//alert('第'+(i+1)+'路，播放失败！');
				e2++;
			}
		}
	}
	if (e1+e2>0){
		alert('提示：'+e2+'路播放失败，'+e1+'路无操作权限。');
	}
}
/*************************************************
Function:		StopRealPlay
Description:	停止预览
Input:			iChannelNum : 通道号
Output:			无
return:			无
*************************************************/
function StopRealPlay(){
	if(HWP.Stop() != 0) {
		alert('停止回放失败');
		return;
	}
}
/*************************************************
Function:		PluginEventHandler
Description:	回放事件响应
Input:			iEventType 事件类型, iParam1 参数1, iParam2 保留
Output:			无
return:			无
*************************************************/
function PluginEventHandler(iEventType,	iParam1, iParam2) {
	if(21 == iEventType) {
		if(m_bIsDiskFreeSpaceEnough) {
			m_bIsDiskFreeSpaceEnough = false;
		    setTimeout(function() {alert('剩余空间不足');}, 2000);
		}
		StopRecord();
	}
}
/*************************************************
Function:		GetSelectWndInfo
Description:	获取选中窗口信息
Input:			SelectWndInfo:窗口信息xml
Output:			无
return:			无
*************************************************/
function GetSelectWndInfo(SelectWndInfo) {
	return;
}

/*************************************************
Function:		size4to3
Description:	插件按照4:3显示
Input:			无			
Output:			无
return:			无				
*************************************************/
function size4to3() {
	$("#content").width(938);
	$("#dvChangeSize").width(938);
	$("#main_plugin").width(938).height(703);
	$("#toolbar").width(938);

	$("#sp4to3").removeClass().addClass("size4to3over");
	$("#sp16to9").removeClass().addClass("size16to9out");
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

	$("#sp4to3").removeClass().addClass("size4to3out");
	$("#sp16to9").removeClass().addClass("size16to9over");
}
/*************************************************
Function:		streamChoose
Description:	主子码流选择
Input:			iStreamType:0-主码流，1-子码流			
Output:			无
return:			无				
*************************************************/
function streamChoose(iStreamType) {
	if(m_iStreamType !== iStreamType) {
	    m_iStreamType = iStreamType;
	    if(HWP.isPlaying()){
		StartRealPlay(); // do stop
		StartRealPlay(); // do start again
	    }
	    if(m_iStreamType==0) size16to9();
	    else size4to3()
	}
	if(m_iStreamType === 0)	{
		$("#substream").removeClass().addClass("substreamout");
		$("#mainstream").removeClass().addClass("mainstreamover");
	} else {
		$("#substream").removeClass().addClass("substreamover");
		$("#mainstream").removeClass().addClass("mainstreamout");		
	}
}
/*************************************************
Function:		screenChoose
Description:	主子码流选择
Input:			iScreenType:0-菜单，1-全屏
Output:			无
return:			无				
*************************************************/
function screenChoose(iScreenType) {
	if(iScreenType === 0)	{
		window.location.href="/live2";
	} else {
		window.location.href="/live2?full=1";
	}
}
