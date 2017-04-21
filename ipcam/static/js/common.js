var m_1stStream	= "/PSIA/streaming/channels/101"; // 主码流uri
var m_2ndStream	= "/PSIA/streaming/channels/102"; // 次码流uri

var m_PreviewOCX=null;
var m_szHostName = new Array();
var m_lHttpPort="80";
var m_lHttp="http://";
var m_lRtspPort="554";
var g_szHttpPort = "80";   //当前http方式的端口，不包括https
var m_szUserPwdValue = new Array();
var m_playbackStream = new Array();
var m_iStreamType = 0; //码流类型: 0-主码流， 1-次码流  3-回放码流 (only for playback.js)

var HWP = null;  //插件对象

/*************************************************
Function:		replaceAll
Description:	替换所有
Input:			szDir:源字符
				szTar:目标字符
Output:			无
return:			无
*************************************************/
String.prototype.replaceAll = function(szDir,szTar)
{
	return this.replace(new RegExp(szDir,"g"), szTar);
}
/*************************************************
Function:		toHex
Description:	转换为16进制
Input:			szStr:源字符
Output:			无
return:			无
*************************************************/
String.prototype.toHex = function()
{
	var szRes = "";
	for(var i = 0; i < this.length; i++)
	{
		szRes += this.charCodeAt(i).toString(16);
	}
	return szRes;
}

/*************************************************
Function:		parseXml
Description:	从xml文件中解析xml
Input:			无
Output:			无
return:			xmlDoc				
*************************************************/
function parseXml(fileRoute)
{
	return $.ajax({
		url: fileRoute,
		dataType: "xml",
		type: "get",
		async: false
	}).responseXML;
}

/*************************************************
Function:		parseXmlFromStr
Description:	从xml字符串中解析xml
Input:			szXml xml字符串
Output:			无
return:			xml文档				
*************************************************/
function parseXmlFromStr(szXml)
{
	if(null == szXml || '' == szXml)
	{
		return null;
	}
	var xmlDoc=new createxmlDoc();
	if(!$.browser.msie)
	{
		var oParser = new DOMParser();
		xmlDoc = oParser.parseFromString(szXml,"text/xml");
	}
	else
	{
		xmlDoc.loadXML(szXml);
	}
	return xmlDoc;
}
/*************************************************
Function:		xmlToStr
Description:	xml转换字符串
Input:			Xml xml文档
Output:			无
return:			字符串				
*************************************************/
function xmlToStr(Xml)
{
	if(Xml == null)
	{
	    return;	
	}
	var XmlDocInfo = '';
	if(navigator.appName == "Netscape" || navigator.appName == "Opera")
	{
		var oSerializer = new XMLSerializer();
		XmlDocInfo = oSerializer.serializeToString(Xml);
	}
	else
	{
		XmlDocInfo = Xml.xml;
	}
	if(XmlDocInfo.indexOf('<?xml') == -1)
	{
		XmlDocInfo = "<?xml version='1.0' encoding='utf-8'?>" + XmlDocInfo;
	}
	return XmlDocInfo;
}

/*************************************************
Function:		Base64
Description:	Base64加密解密
Input:			无		
Output:			无
return:			无				
*************************************************/
 var Base64 = {
 
	// private property
	_keyStr : "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=",
 
	// public method for encoding
	encode : function (input) {
		var output = "";
		var chr1, chr2, chr3, enc1, enc2, enc3, enc4;
		var i = 0;
 
		input = Base64._utf8_encode(input);
 
		while (i < input.length) {
 
			chr1 = input.charCodeAt(i++);
			chr2 = input.charCodeAt(i++);
			chr3 = input.charCodeAt(i++);
 
			enc1 = chr1 >> 2;
			enc2 = ((chr1 & 3) << 4) | (chr2 >> 4);
			enc3 = ((chr2 & 15) << 2) | (chr3 >> 6);
			enc4 = chr3 & 63;
 
			if (isNaN(chr2)) {
				enc3 = enc4 = 64;
			} else if (isNaN(chr3)) {
				enc4 = 64;
			}
 
			output = output +
			this._keyStr.charAt(enc1) + this._keyStr.charAt(enc2) +
			this._keyStr.charAt(enc3) + this._keyStr.charAt(enc4);
 
		}
 
		return output;
	},
 
	// public method for decoding
	decode : function (input) {
		var output = "";
		var chr1, chr2, chr3;
		var enc1, enc2, enc3, enc4;
		var i = 0;
 
		input = input.replace(/[^A-Za-z0-9\+\/\=]/g, "");
 
		while (i < input.length) {
 
			enc1 = this._keyStr.indexOf(input.charAt(i++));
			enc2 = this._keyStr.indexOf(input.charAt(i++));
			enc3 = this._keyStr.indexOf(input.charAt(i++));
			enc4 = this._keyStr.indexOf(input.charAt(i++));
 
			chr1 = (enc1 << 2) | (enc2 >> 4);
			chr2 = ((enc2 & 15) << 4) | (enc3 >> 2);
			chr3 = ((enc3 & 3) << 6) | enc4;
 
			output = output + String.fromCharCode(chr1);
 
			if (enc3 != 64) {
				output = output + String.fromCharCode(chr2);
			}
			if (enc4 != 64) {
				output = output + String.fromCharCode(chr3);
			}
 
		}
 
		output = Base64._utf8_decode(output);
 
		return output;
 
	},
 
	// private method for UTF-8 encoding
	_utf8_encode : function (string) {
		string = string.replace(/\r\n/g,"\n");
		var utftext = "";
 
		for (var n = 0; n < string.length; n++) {
 
			var c = string.charCodeAt(n);
 
			if (c < 128) {
				utftext += String.fromCharCode(c);
			}
			else if((c > 127) && (c < 2048)) {
				utftext += String.fromCharCode((c >> 6) | 192);
				utftext += String.fromCharCode((c & 63) | 128);
			}
			else {
				utftext += String.fromCharCode((c >> 12) | 224);
				utftext += String.fromCharCode(((c >> 6) & 63) | 128);
				utftext += String.fromCharCode((c & 63) | 128);
			}
 
		}
 
		return utftext;
	},
 
	// private method for UTF-8 decoding
	_utf8_decode : function (utftext) {
		var string = "";
		var i = 0;
		var c = c1 = c2 = 0;
 
		while ( i < utftext.length ) {
 
			c = utftext.charCodeAt(i);
 
			if (c < 128) {
				string += String.fromCharCode(c);
				i++;
			}
			else if((c > 191) && (c < 224)) {
				c2 = utftext.charCodeAt(i+1);
				string += String.fromCharCode(((c & 31) << 6) | (c2 & 63));
				i += 2;
			}
			else {
				c2 = utftext.charCodeAt(i+1);
				c3 = utftext.charCodeAt(i+2);
				string += String.fromCharCode(((c & 15) << 12) | ((c2 & 63) << 6) | (c3 & 63));
				i += 3;
			}
 
		}
 
		return string;
	} 
}
/*************************************************
Function:		checkPlugin
Description:	检测是否安装插件及向页面插入插件元素
Input:			iType:表示插入插件的位置，0表示本地配置或远程升级等，1表示字符叠加等，2表示预览节界面
				szInfo:提示信息  该参数无效  modified by chenxiangzhen 2012-02-17
				iWndType:窗口分割模式
Output:			true:插件已安装；false:插件未安装
return:			无				
*************************************************/
function checkPlugin(iType, szInfo, iWndType, szPlayMode)
{
	if (!$.browser.msie)
	{
		if ($("#main_plugin").html() !== "" && $("#PreviewActiveX").length !== 0)
		{
			var iPlayMode = 0;
			switch (szPlayMode)
			{
				case "normal":
					//iPlayMode = 0;
					break;
				case "motiondetect":
					iPlayMode = 1;
					break;
				case "tamperdetect":
					iPlayMode = 2;
					break;
				case "privacymask":
					iPlayMode = 3;
					break;
				case "textoverlay":
					iPlayMode = 4;
					break;
				case "osdsetting":
					iPlayMode = 5;
					break;
				default:
					//iPlayMode = 0;
					break;
			}
			HWP.SetPlayModeType(iPlayMode);
			HWP.ArrangeWindow(parseInt(iWndType));
			return true;
		}
		
		var bInstalled = false;
		for (var i = 0, len = navigator.mimeTypes.length; i < len; i++)
		{
			if (navigator.mimeTypes[i].type.toLowerCase() == "application/hwp-webvideo-plugin")
			{
				bInstalled = true;
				if (iType == '0')
				{
				    $("#main_plugin").html("<embed type='application/hwp-webvideo-plugin' id='PreviewActiveX' width='1' height='1' name='PreviewActiveX' align='center' wndtype='"+iWndType+"' playmode='"+szPlayMode+"'>");
					setTimeout(function() { $("#PreviewActiveX").css('height','0px'); }, 10); // 避免插件初始化不完全
				}
				else if (iType == '1')
				{
					$("#main_plugin").html("<embed type='application/hwp-webvideo-plugin' id='PreviewActiveX' width='352' height='288' name='PreviewActiveX' align='center' wndtype='"+iWndType+"' playmode='"+szPlayMode+"'>");
				}
				else
				{
					$("#main_plugin").html("<embed type='application/hwp-webvideo-plugin' id='PreviewActiveX' width='100%' height='100%' name='PreviewActiveX' align='center' wndtype='"+iWndType+"' playmode='"+szPlayMode+"'>");
				}
				$("#PreviewActiveX").css('width','99.99%');
				break;
			}
		}
		if (!bInstalled)
		{
			if (navigator.platform == "Win32")
			{
				$("#main_plugin").html("<label name='laPlugin' onclick='window.open(\"/static/codebase/WebComponents.exe\",\"_self\")' class='pluginLink' onMouseOver='this.className =\"pluginLinkSel\"' onMouseOut='this.className =\"pluginLink\"'>请点击此处下载插件，安装时请先关闭浏览器</label>");
			}
			else if (navigator.platform == "Mac68K" || navigator.platform == "MacPPC" || navigator.platform == "Macintosh")
			{
				$("#main_plugin").html("<label name='laNotWin32Plugin' onclick='' class='pluginLink' style='cursor:default; text-decoration:none;'>未检测到插件</label>");
			}
			else
			{
				$("#main_plugin").html("<label name='laNotWin32Plugin' onclick='' class='pluginLink' style='cursor:default; text-decoration:none;'>未检测到插件</label>");
			}
		  	return false;
		}
	}
	else
	{
		if ($("#main_plugin").html() !== "" && $("#PreviewActiveX").length !== 0 && $("#PreviewActiveX")[0].object !== null)
		{
			var iPlayMode = 0;
			switch (szPlayMode)
			{
				case "normal":
					//iPlayMode = 0;
					break;
				case "motiondetect":
					iPlayMode = 1;
					break;
				case "tamperdetect":
					iPlayMode = 2;
					break;
				case "privacymask":
					iPlayMode = 3;
					break;
				case "textoverlay":
					iPlayMode = 4;
					break;
				case "osdsetting":
					iPlayMode = 5;
					break;
				default:
					//iPlayMode = 0;
					break;
			}
			HWP.SetPlayModeType(iPlayMode);
			HWP.ArrangeWindow(parseInt(iWndType));
			return true;
		}
		
		$("#main_plugin").html("<object classid='clsid:E7EF736D-B4E6-4A5A-BA94-732D71107808' codebase='' standby='Waiting...' id='PreviewActiveX' width='100%' height='100%' name='ocx' align='center' ><param name='wndtype' value='"+iWndType+"'><param name='playmode' value='"+szPlayMode+"'></object>");
		var previewOCX=document.getElementById("PreviewActiveX");
		if(previewOCX == null || previewOCX.object == null)
		{
			if((navigator.platform == "Win32"))
			{
				$("#main_plugin").html("<label name='laPlugin' onclick='window.open(\"/static/codebase/WebComponents.exe\",\"_self\")' class='pluginLink' onMouseOver='this.className =\"pluginLinkSel\"' onMouseOut='this.className =\"pluginLink\"'>请点击此处下载插件，安装时请先关闭浏览器<label>");
			}
			else
			{
				$("#main_plugin").html("<label name='laNotWin32Plugin' onclick='' class='pluginLink' style='cursor:default; text-decoration:none;'>未检测到插件<label>");
			}
			
		  return false;
		}
	}
	return true;
}
/*************************************************
Function:		CompareFileVersion
Description:	比较文件版本
Input:			无
Output:			无
return:			false:需要更新 true：不需要更新
*************************************************/
function CompareFileVersion()
{
	var previewOCX=document.getElementById("PreviewActiveX");
	if(previewOCX == null)
	{
		return false;
	}
	var xmlDoc=parseXml("/static/xml/version.xml");
	var szXml = xmlToStr(xmlDoc);
	var bRes = false;
	try
	{
		bRes = !previewOCX.HWP_CheckPluginUpdate(szXml);
		return bRes;
	}
	catch(e)
	{
		if(m_szBrowser != 'Netscape')
		{
			if(1 == CompareVersion("WebVideoActiveX.ocx"))
			{
				return false;		//插件需要更新
			}
		}
		else
		{
			if(1 == CompareVersion("npWebVideoPlugin.dll"))
			{
				return false;		//插件需要更新
			}
		}
		
		if(1 == CompareVersion("PlayCtrl.dll"))
		{
			return false;		//插件需要更新
		}
		
		if(1 == CompareVersion("StreamTransClient.dll"))
		{
			return false;		//插件需要更新
		}
		
		if(1 == CompareVersion("NetStream.dll"))
		{
			return false;		//插件需要更新
		}
		
		if(1 == CompareVersion("SystemTransform.dll"))
		{
			return false;		//插件需要更新
		}
		
		return true;
	}
}
/*************************************************
Function:		CompareVersion
Description:	比较文件版本
Input:			文件名
Output:			无
return:			-1 系统中的版本高 0 版本相同 1 设备中的版本高
*************************************************/
function CompareVersion(szFileName)
{
	var xmlDoc=parseXml("/static/xml/version.xml");
	
	var fvOld = m_PreviewOCX.GetFileVersion(szFileName,"FileVersion");
	var fvNew = xmlDoc.documentElement.getElementsByTagName(szFileName)[0].childNodes[0].nodeValue;
	
	if(szFileName == "hpr.dll")
	{
		var sp = ".";
	}
	else
	{
		var sp = ",";
	}
	var fvSigleOld = fvOld.split(sp);
	var fvSigleNew = fvNew.split(sp);
	
	for(var i = 0;i < 4;i++)
	{
		if(parseInt(fvSigleOld[i]) > parseInt(fvSigleNew[i]))
		{
			return -1;
		}
		
		if(parseInt(fvSigleOld[i]) < parseInt(fvSigleNew[i]))
		{
			return 1;
		}
	}
	return 0;
}
/*************************************************
Function:		getXMLHttpRequest
Description:	创建xmlhttprequest对象
Input:			无			
Output:			无
return:			无				
*************************************************/
function getXMLHttpRequest()    
{
        var xmlHttpRequest = null; 
        if (window.XMLHttpRequest) 
        {
            xmlHttpRequest = new XMLHttpRequest();
        }
        else if (window.ActiveXObject)
        {
        	xmlHttpRequest = new ActiveXObject("Microsoft.XMLHTTP");
        } 
     	return xmlHttpRequest;
} 
/*************************************************
Function:		createxmlDoc
Description:	创建xml DOM对象
Input:			无			
Output:			无
return:			无				
*************************************************/
function createxmlDoc()
{
	var xmlDoc;
	var aVersions = [ "MSXML2.DOMDocument","MSXML2.DOMDocument.5.0",
	"MSXML2.DOMDocument.4.0","MSXML2.DOMDocument.3.0",
	"Microsoft.XmlDom"];
	
	for (var i = 0; i < aVersions.length; i++) 
	{
		try 
		{
			xmlDoc = new ActiveXObject(aVersions[i]);
			break;
		}
		catch (oError)
		{
			xmlDoc = document.implementation.createDocument("", "", null);
			break;
		}
	}
	xmlDoc.async="false";
	return xmlDoc;
}

/**********************************
Function:		DayAdd
Description:	日期加天数
Input:			szDay: 要加的日期
				iAdd： 加的天数
Output:			无
return:			true 有录像； false 没有录像；		
***********************************/
function DayAdd(szDay,iAdd)
{
	var date =  new Date(Date.parse(szDay.replace(/\-/g,'/')));
	var newdate = new Date(date.getTime()+(iAdd*24 * 60 * 60 * 1000));
	
	return newdate.Format("yyyy-MM-dd hh:mm:ss");   
}

// 对Date的扩展，将 Date 转化为指定格式的String
// 月(M)、日(d)、小时(h)、分(m)、秒(s)、季度(q) 可以用 1-2 个占位符，
// 年(y)可以用 1-4 个占位符，毫秒(S)只能用 1 个占位符(是 1-3 位的数字)
// 例子：
// (new Date()).Format("yyyy-MM-dd hh:mm:ss.S") ==> 2006-07-02 08:09:04.423
// (new Date()).Format("yyyy-M-d h:m:s.S")      ==> 2006-7-2 8:9:4.18
Date.prototype.Format = function (fmt)
{
	var o=
	{
		"M+":this.getMonth()+1,//月份
		"d+":this.getDate(),//日
		"h+":this.getHours(),//小时
		"m+":this.getMinutes(),//分
		"s+":this.getSeconds(),//秒
		"q+":Math.floor((this.getMonth()+3)/3),//季度
		"S":this.getMilliseconds()//毫秒
	};
	if(/(y+)/.test(fmt))
	fmt=fmt.replace(RegExp.$1,(this.getFullYear()+"").substr(4-RegExp.$1.length));
	for(var k in o)
	if(new RegExp("("+k+")").test(fmt))
	fmt=fmt.replace(RegExp.$1,(RegExp.$1.length==1)?(o[k]):(("00"+o[k]).substr((""+o[k]).length)));
	return fmt;
}
/*************************************************
Function:		UpdateTips
Description:	更新提示
Input:			无
Output:			无
return:			无
*************************************************/
function UpdateTips()
{
	var bUpdateTips = $.cookie('updateTips');
	var szUpdate = '';
	if(bUpdateTips == 'true'){
		if(navigator.platform == "Win32") {
			szUpdate = "插件检测到新版本，是否更新？";
			Warning =confirm(szUpdate);
			if (Warning) {
				window.open("/static/codebase/WebComponents.exe","_self");
			} else {
				$.cookie('updateTips', 'false');
			}
		} else {
			szUpdate = '插件检测到新版本，请与供应商联系更新。';
			setTimeout(function() {alert(szUpdate);},20);
			$.cookie('updateTips', 'false');
		}
	}
}
/*************************************************
Function:		isIPv6Add
Description:	校验是否为有效的IPV6地址
Input:			strInfo:IPV6地址
Output:			true:是 false:否
return:			无
*************************************************/
function  isIPv6Add(strInfo)
{
	  return /:/.test(strInfo) && strInfo.match(/:/g).length<8 && /::/.test(strInfo)?(strInfo.match(/::/g).length==1 && /^::$|^(::)?([\da-f]{1,4}(:|::))*[\da-f]{1,4}(:|::)?$/i.test(strInfo)):/^([\da-f]{1,4}:){7}[\da-f]{1,4}$/i.test(strInfo);
}