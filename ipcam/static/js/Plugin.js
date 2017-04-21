/*****************************************************
Copyright 2007-2011 Hikvision Digital Technology Co., Ltd.   
FileName: Plugin.js
Description: 插件管理类
Author: wuyang    
Date: 2011.12.27
*****************************************************/

function Plugin(iWndNum, szIP, szHttpPort, szRtspPort) {
	this.iWndNum = iWndNum; // 子窗体个数
	this.iProtocolType = 0; // 取流方式，默认为RTSP
	this.wnds = new Array(this.iWndNum);
	var that = this;
	$.each(this.wnds, function(iWndNo) {
		that.wnds[iWndNo] = {
			isPlaying: false,
			isPause: false
		};
	});
	this.isPlaying = function() {
		var ret = false;
		$.each(this.wnds, function(iWndNo, wnd) {
			if (wnd.isPlaying) {
				ret = true;
				return false;
			}
		});
		return ret;
	};
	this.isPause = function() {
		var ret = false;
		$.each(this.wnds, function(iWndNo, wnd) {
			if (wnd.isPlaying && wnd.isPause) {
				ret = true;
				return false;
			}
		});
		return ret;
	};
	this.destory = function() {
		this.Stop();
		$("#main_plugin").empty();
	};
	this.ArrangeWindow = function(iWndType) {
		try {
			$("#PreviewActiveX")[0].HWP_ArrangeWindow(iWndType);
		} catch (e) {}
	}
	this.Play = function(iWndNo) {
		if (arguments.length === 0) {
			iWndNo = 0;
		}
		if (this.wnds[iWndNo].isPlaying) {
			return 0;
		}
		try
		{
			var previewOCX = $("#PreviewActiveX")[0];
			if (m_iStreamType==3){
				var szURL = "rtsp://" + szIP[iWndNo] + ":" + m_lRtspPort + m_playbackStream[iWndNo];
			} else {
				var szURL = "rtsp://" + szIP[iWndNo] + ":" + szRtspPort + ((m_iStreamType==0)?m_1stStream:m_2ndStream);
			}
			var iRet = previewOCX.HWP_Play(szURL, m_szUserPwdValue[iWndNo], iWndNo, "", "");
			if (iRet === 0) {
				this.wnds[iWndNo].isPlaying = true;
				that.wnds[iWndNo].isPause = false;
			}
			return iRet;
		} catch (e) { return -1; }
	};
	this.Stop = function(iWndNo) {
		function Stop(iWndNo) {
			if (!that.wnds[iWndNo].isPlaying) {
				return 0;
			}
			that.wnds[iWndNo].isPlaying = false;
			that.wnds[iWndNo].isPause = false;
			try {
				return $("#PreviewActiveX")[0].HWP_Stop(iWndNo);
			} catch (e) { return -1; }
		}
		
		if (arguments.length === 0) {
			var iRet = 0;
			$.each(this.wnds, function(iWndNo, wnd) {
				if (Stop(iWndNo) !== 0) {
					iRet = -1;
				}
			});
			return iRet;
		} else {
			return Stop(iWndNo);
		}
	};
	this.Pause = function(iWndNo) {
		function Pause(iWndNo) {
			if (!that.wnds[iWndNo].isPlaying) {
				return 0;
			}
			that.wnds[iWndNo].isPause = true;
			try {
				return $("#PreviewActiveX")[0].HWP_Pause(iWndNo);
			} catch (e) { return -1; }
		}
		
		if (arguments.length === 0) {
			var iRet = 0;
			$.each(this.wnds, function(iWndNo, wnd) {
				if (Pause(iWndNo) !== 0) {
					iRet = -1;
				}
			});
			return iRet;
		} else {
			return Pause(iWndNo);
		}
	};
	this.Resume = function(iWndNo) {
		function Resume(iWndNo) {
			if (!that.wnds[iWndNo].isPlaying) {
				return 0;
			}
			that.wnds[iWndNo].isPause = false;
			try {
				return $("#PreviewActiveX")[0].HWP_Resume(iWndNo);
			} catch (e) { return -1; }
		}
		
		if (arguments.length === 0) {
			var iRet = 0;
			$.each(this.wnds, function(iWndNo, wnd) {
				if (Resume(iWndNo) !== 0) {
					iRet = -1;
				}
			});
			return iRet;
		} else {
			return Resume(iWndNo);
		}
	};
	this.SetDrawStatus = function(bStartDraw) {
		try {
			return $("#PreviewActiveX")[0].HWP_SetDrawStatus(bStartDraw);
		} catch (e) { return -1; }
	};
	this.GetRegionInfo = function() {
		try {
			return $("#PreviewActiveX")[0].HWP_GetRegionInfo();
		} catch (e) {
			return "";
		}
	};
	this.SetRegionInfo = function(szRegionInfo) {
		try {
			return $("#PreviewActiveX")[0].HWP_SetRegionInfo(szRegionInfo);
		} catch (e) { return -1; }
	};
	this.ClearRegion = function() {
		try {
			return $("#PreviewActiveX")[0].HWP_ClearRegion();
		} catch (e) { return -1; }
	};
	this.GetTextOverlay = function() {
		try {
			return $("#PreviewActiveX")[0].HWP_GetTextOverlay();
		} catch (e) {
			return "";
		}
	};
	this.SetTextOverlay = function(szTextOverlay) {
		try {
			return $("#PreviewActiveX")[0].HWP_SetTextOverlay(szTextOverlay);
		} catch (e) { return -1; }
	};
	this.SetPlayModeType = function(iPlayMode) {
		try {
			return $("#PreviewActiveX")[0].HWP_SetPlayModeType(iPlayMode);
		} catch (e) { return -1; }
	};
}