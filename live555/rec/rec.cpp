// Copyright (c) 1996-2014, Live Networks, Inc.  All rights reserved
// A demo application, showing how to create and run a RTSP client (that can potentially receive multiple streams concurrently).
//
// NOTE: This code - although it builds a running application - is intended only to illustrate how to develop your own RTSP
// client application.  For a full-featured RTSP client application - with much more functionality, and many options - see
// "openRTSP": http://www.live555.com/openRTSP/

#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "liveMedia.hh"
#include "BasicUsageEnvironment.hh"

#include "rec.hh"

extern "C" {
#include "kam.h"
}

#define	RTSP_PORT 554
#define CHECK_K_DELAY 10

// If you don't want to see debugging output for each received frame, then comment out the following line:
//#define DEBUG_PRINT_EACH_RECEIVED_FRAME 1
//#define DEBUG
// by default, print verbose output from each "RTSPClient"
#define RTSP_CLIENT_VERBOSITY_LEVEL 0
// request that the server stream its data using RTP or UDP.
#define REQUEST_STREAMING_OVER_TCP True
// Define the size of the buffer that will receive frame
#define DUMMY_SINK_RECEIVE_BUFFER_SIZE 500000
// Define the size of the frame cache
#define FRAME_CACHE_BUFFER_SIZE 1000000

char eventLoopWatchVariable=0;
Authenticator* ourAuthenticator = NULL;
char username[15] = "__base64__";
static unsigned rtspClientCount = 0; // Counts how many streams (i.e., "RTSPClient"s) are currently in use.


// A function that outputs a string that identifies each stream (for debugging output).  Modify this if you wish:
UsageEnvironment& operator<<(UsageEnvironment& env, const RTSPClient& rtspClient) {
  return env << "[URL:\"" << rtspClient.url() << "\"]: ";
}

// A function that outputs a string that identifies each subsession (for debugging output).  Modify this if you wish:
UsageEnvironment& operator<<(UsageEnvironment& env, const MediaSubsession& subsession) {
  return env << subsession.mediumName() << "/" << subsession.codecName();
}

void *snap_thread(void *arg)
{
	puts("snap: thread started.");
	
	while(1){
		if (k_delay>0) /* start to recorde video */
			rec_run();
#ifdef DEBUG
		else{
			fprintf(stderr, "snap: thread sleep ...\n");
		}
#endif
		sleep(CHECK_K_DELAY);
	}
	
	puts("snap: thread exit.");  
	
	return NULL;
}

void rec_run(void)
{
  char rtsp_req[100];
  
  puts("rec_run(): started.");
  
  // Begin by setting up our usage environment:
  TaskScheduler* scheduler = BasicTaskScheduler::createNew();
  UsageEnvironment* env = BasicUsageEnvironment::createNew(*scheduler);

  ourAuthenticator = new Authenticator(username, k_auth);

  // Open and start streaming:
  sprintf(rtsp_req, "rtsp://%s:%d/PSIA/streaming/channels/101", k_kamip, RTSP_PORT);
  openURL(*env, "", rtsp_req);

  // All subsequent activity takes place within the event loop:
  eventLoopWatchVariable = 0;
  env->taskScheduler().doEventLoop(&eventLoopWatchVariable);
    // This function call does not return, unless, at some point in time, "eventLoopWatchVariable" gets set to something non-zero.
  
  env->reclaim(); env = NULL;
  delete scheduler; scheduler = NULL;
  delete ourAuthenticator;
  
  puts("rec_run(): exit.");
  
  return;
}

void openURL(UsageEnvironment& env, char const* progName, char const* rtspURL) {
  // Begin by creating a "RTSPClient" object.  
  RTSPClient* rtspClient = ourRTSPClient::createNew(env, rtspURL, RTSP_CLIENT_VERBOSITY_LEVEL, progName);
  if (rtspClient == NULL) {
    env << "Failed to create a RTSP client for URL \"" << rtspURL << "\": " << env.getResultMsg() << "\n";
    return;
  }

  ++rtspClientCount;

  // Next, send a RTSP "DESCRIBE" command, to get a SDP description for the stream.
  // Note that this command - like all RTSP commands - is sent asynchronously; we do not block, waiting for a response.
  // Instead, the following function call returns immediately, and we handle the RTSP response later, from within the event loop:
  rtspClient->sendDescribeCommand(continueAfterDESCRIBE, ourAuthenticator); 
}


// Implementation of the RTSP 'response handlers':

void continueAfterDESCRIBE(RTSPClient* rtspClient, int resultCode, char* resultString) {
  do {
    UsageEnvironment& env = rtspClient->envir(); // alias
    StreamClientState& scs = ((ourRTSPClient*)rtspClient)->scs; // alias

    if (resultCode != 0) {
      env << *rtspClient << "Failed to get a SDP description: " << resultString << "\n";
      delete[] resultString;
      break;
    }
    // -------------------------------------------------------------------------------------
    // add these for Hicomm camera, to handle channel_id=0x04 packet (pyload_type=0x70)
    //
    char const sdp_more[100]=	"m=plain 0 RTP/AVP 112\r\n" \
    				"a=control:trackID=2\r\n" \
    				"a=rtpmap:112 DATA/1000\r\n" \
    				"a=fmtp:112\r\n";
    // -------------------------------------------------------------------------------------
    char* sdpDescription = new char[strlen(resultString)+strlen(sdp_more)+10]; //resultString;
    sprintf(sdpDescription, "%s%s", resultString, sdp_more);
    env << *rtspClient << "Got a SDP description:\n" << sdpDescription << "\n";

    // Create a media session object from this SDP description:
    scs.session = MediaSession::createNew(env, sdpDescription);
    delete[] sdpDescription; // because we don't need it anymore
    delete[] resultString;
    if (scs.session == NULL) {
      env << *rtspClient << "Failed to create a MediaSession object from the SDP description: " << env.getResultMsg() << "\n";
      break;
    } else if (!scs.session->hasSubsessions()) {
      env << *rtspClient << "This session has no media subsessions (i.e., no \"m=\" lines)\n";
      break;
    }

    // Then, create and set up our data source objects for the session.  We do this by iterating over the session's 'subsessions',
    // calling "MediaSubsession::initiate()", and then sending a RTSP "SETUP" command, on each one.
    // (Each 'subsession' will have its own data source.)
    scs.iter = new MediaSubsessionIterator(*scs.session);
    setupNextSubsession(rtspClient);
    return;
  } while (0);

  // An unrecoverable error occurred with this stream.
  shutdownStream(rtspClient);
}


void setupNextSubsession(RTSPClient* rtspClient) {
  UsageEnvironment& env = rtspClient->envir(); // alias
  StreamClientState& scs = ((ourRTSPClient*)rtspClient)->scs; // alias
  
  scs.subsession = scs.iter->next();
  if (scs.subsession != NULL) {
    Boolean init_OK;
    if (strcmp(scs.subsession->codecName(), "DATA")==0)
	init_OK=scs.subsession->initiate(0);
    else
    	init_OK=scs.subsession->initiate();
    if (!init_OK) {
      env << *rtspClient << "Failed to initiate the \"" << *scs.subsession << "\" subsession: " << env.getResultMsg() << "\n";
      setupNextSubsession(rtspClient); // give up on this subsession; go to the next one
    } else {
      env << *rtspClient << "Initiated the \"" << *scs.subsession << "\" subsession\n";

      // Continue setting up this subsession, by sending a RTSP "SETUP" command:
      rtspClient->sendSetupCommand(*scs.subsession, continueAfterSETUP, False, REQUEST_STREAMING_OVER_TCP, False, ourAuthenticator);
    }
    return;
  }

  // We've finished setting up all of the subsessions.  Now, send a RTSP "PLAY" command to start the streaming:
  scs.duration = CHECK_K_DELAY; 
  if (scs.session->absStartTime() != NULL) {
    // Special case: The stream is indexed by 'absolute' time, so send an appropriate "PLAY" command:
    rtspClient->sendPlayCommand(*scs.session, continueAfterPLAY, scs.session->absStartTime(), scs.session->absEndTime(), 1.0f, ourAuthenticator);
  } else {
    rtspClient->sendPlayCommand(*scs.session, continueAfterPLAY, 0.0f, 0.0f, 1.0f, ourAuthenticator);
  }
}

void continueAfterSETUP(RTSPClient* rtspClient, int resultCode, char* resultString) {
  do {
    UsageEnvironment& env = rtspClient->envir(); // alias
    StreamClientState& scs = ((ourRTSPClient*)rtspClient)->scs; // alias

    if (resultCode != 0) {
      env << *rtspClient << "Failed to set up the \"" << *scs.subsession << "\" subsession: " << resultString << "\n";
      break;
    }

    env << *rtspClient << "Set up the \"" << *scs.subsession
#if REQUEST_STREAMING_OVER_TCP==True
	<< "\" subsession (channel " << scs.subsession->rtpChannelId << "-" << scs.subsession->rtcpChannelId << ")\n";
#else
	<< "\" subsession (client ports " << scs.subsession->clientPortNum() << "-" << scs.subsession->clientPortNum()+1 << ")\n";
#endif

    // Having successfully setup the subsession, create a data sink for it, and call "startPlaying()" on it.
    // (This will prepare the data sink to receive data; the actual flow of data from the client won't start happening until later,
    // after we've sent a RTSP "PLAY" command.)

    scs.subsession->sink = DummySink::createNew(env, scs, rtspClient->url());
      // perhaps use your own custom "MediaSink" subclass instead
    if (scs.subsession->sink == NULL) {
      env << *rtspClient << "Failed to create a data sink for the \"" << *scs.subsession
	  << "\" subsession: " << env.getResultMsg() << "\n";
      break;
    }

    env << *rtspClient << "Created a data sink for the \"" << *scs.subsession << "\" subsession\n";
    scs.subsession->miscPtr = rtspClient; // a hack to let subsession handle functions get the "RTSPClient" from the subsession 
    scs.subsession->sink->startPlaying(*(scs.subsession->readSource()),
				       subsessionAfterPlaying, scs.subsession);
    // Also set a handler to be called if a RTCP "BYE" arrives for this subsession:
    if (scs.subsession->rtcpInstance() != NULL) {
      scs.subsession->rtcpInstance()->setByeHandler(subsessionByeHandler, scs.subsession);
    }
  } while (0);
  delete[] resultString;

  // Set up the next subsession, if any:
  setupNextSubsession(rtspClient);
}

void continueAfterPLAY(RTSPClient* rtspClient, int resultCode, char* resultString) {
  Boolean success = False;

  do {
    UsageEnvironment& env = rtspClient->envir(); // alias
    StreamClientState& scs = ((ourRTSPClient*)rtspClient)->scs; // alias

    if (resultCode != 0) {
      env << *rtspClient << "Failed to start playing session: " << resultString << "\n";
      break;
    }

    // Set a timer to be handled at the end of the stream's expected duration (if the stream does not already signal its end
    // using a RTCP "BYE").  This is optional.  If, instead, you want to keep the stream active - e.g., so you can later
    // 'seek' back within it and do another RTSP "PLAY" - then you can omit this code.
    // (Alternatively, if you don't want to receive the entire stream, you could set this timer for some shorter value.)
    if (scs.duration > 0) {
      unsigned uSecsToDelay = (unsigned)(scs.duration*1000000);
      scs.streamTimerTask = env.taskScheduler().scheduleDelayedTask(uSecsToDelay, (TaskFunc*)streamTimerHandler, rtspClient);
    }

    env << *rtspClient << "Started playing session";
    if (scs.duration > 0) {
      env << " (for up to " << scs.duration << " seconds)";
    }
    env << "...\n";

    success = True;
  } while (0);
  delete[] resultString;

  if (!success) {
    // An unrecoverable error occurred with this stream.
    shutdownStream(rtspClient);
  }
}


// Implementation of the other event handlers:

void subsessionAfterPlaying(void* clientData) {
  MediaSubsession* subsession = (MediaSubsession*)clientData;
  RTSPClient* rtspClient = (RTSPClient*)(subsession->miscPtr);

  // Begin by closing this subsession's stream:
  Medium::close(subsession->sink);
  subsession->sink = NULL;

  // Next, check whether *all* subsessions' streams have now been closed:
  MediaSession& session = subsession->parentSession();
  MediaSubsessionIterator iter(session);
  while ((subsession = iter.next()) != NULL) {
    if (subsession->sink != NULL) return; // this subsession is still active
  }

  // All subsessions' streams have now been closed, so shutdown the client:
  shutdownStream(rtspClient);
}

void subsessionByeHandler(void* clientData) {
  MediaSubsession* subsession = (MediaSubsession*)clientData;
  RTSPClient* rtspClient = (RTSPClient*)subsession->miscPtr;
  UsageEnvironment& env = rtspClient->envir(); // alias

  env << *rtspClient << "Received RTCP \"BYE\" on \"" << *subsession << "\" subsession\n";

  // Now act as if the subsession had closed:
  subsessionAfterPlaying(subsession);
}

void streamTimerHandler(void* clientData) {
  ourRTSPClient* rtspClient = (ourRTSPClient*)clientData;
  StreamClientState& scs = rtspClient->scs; // alias
  UsageEnvironment& env = rtspClient->envir(); // alias

#ifdef DEBUG
  fprintf(stderr, "TimerHandler(): check k_delay\n");
#endif

  env.taskScheduler().unscheduleDelayedTask(scs.streamTimerTask);
  
  if (k_delay<0){
  	scs.streamTimerTask = NULL;

  	// Shut down the stream:
  	shutdownStream(rtspClient);
  }
  else{
	if (scs.duration > 0) {
		unsigned uSecsToDelay = (unsigned)(scs.duration*1000000);
		scs.streamTimerTask = env.taskScheduler().scheduleDelayedTask(uSecsToDelay, (TaskFunc*)streamTimerHandler, rtspClient);
	}
  }
}

void shutdownStream(RTSPClient* rtspClient, int exitCode) {
  UsageEnvironment& env = rtspClient->envir(); // alias
  StreamClientState& scs = ((ourRTSPClient*)rtspClient)->scs; // alias

  // First, check whether any subsessions have still to be closed:
  if (scs.session != NULL) { 
    Boolean someSubsessionsWereActive = False;
    MediaSubsessionIterator iter(*scs.session);
    MediaSubsession* subsession;
    
    while ((subsession = iter.next()) != NULL) {
      if (subsession->sink != NULL) {
	Medium::close(subsession->sink);
	subsession->sink = NULL;

	if (subsession->rtcpInstance() != NULL) {
	  subsession->rtcpInstance()->setByeHandler(NULL, NULL); // in case the server sends a RTCP "BYE" while handling "TEARDOWN"
	}

	someSubsessionsWereActive = True;
      }
    }

    if (someSubsessionsWereActive) {
      // Send a RTSP "TEARDOWN" command, to tell the server to shutdown the stream.
      // Don't bother handling the response to the "TEARDOWN".
      rtspClient->sendTeardownCommand(*scs.session, NULL, ourAuthenticator);
    }
  }

  env << *rtspClient << "Closing the stream.\n";
  Medium::close(rtspClient);
    // Note that this will also cause this stream's "StreamClientState" structure to get reclaimed.

  if (--rtspClientCount == 0) {
    // The final stream has ended, so that we leave the LIVE555 event loop, and continue running "rec_run()".)
    eventLoopWatchVariable = 1;
  }
}


// Implementation of "ourRTSPClient":

ourRTSPClient* ourRTSPClient::createNew(UsageEnvironment& env, char const* rtspURL,
					int verbosityLevel, char const* applicationName, portNumBits tunnelOverHTTPPortNum) {
  return new ourRTSPClient(env, rtspURL, verbosityLevel, applicationName, tunnelOverHTTPPortNum);
}

ourRTSPClient::ourRTSPClient(UsageEnvironment& env, char const* rtspURL,
			     int verbosityLevel, char const* applicationName, portNumBits tunnelOverHTTPPortNum)
  : RTSPClient(env,rtspURL, verbosityLevel, applicationName, tunnelOverHTTPPortNum, -1) {
}

ourRTSPClient::~ourRTSPClient() {
}


// Implementation of "StreamClientState":

StreamClientState::StreamClientState()
  : iter(NULL), session(NULL), subsession(NULL), streamTimerTask(NULL), duration(0.0) {
	frameCache = new u_int8_t[FRAME_CACHE_BUFFER_SIZE]; 
	cacheUsed=0;
	fid=NULL;
	lastTime.tv_sec=0;
	last_dir_n=0;
}

StreamClientState::~StreamClientState() 
{
  delete iter;
  delete[] frameCache;
  if (session != NULL) {
    // We also need to delete "session", and unschedule "streamTimerTask" (if set)
    UsageEnvironment& env = session->envir(); // alias

    env.taskScheduler().unscheduleDelayedTask(streamTimerTask);
    Medium::close(session);
  }
  if (fid != NULL) fclose(fid);
}

void StreamClientState::addCache(unsigned char const* data, unsigned dataSize) 
{
	unsigned toCopy;
	if (cacheUsed+dataSize>FRAME_CACHE_BUFFER_SIZE){
		printf("addCache(): frame cache FULL!\n");
		toCopy=cacheUsed+dataSize-FRAME_CACHE_BUFFER_SIZE;
	}
	else
		toCopy=dataSize;
	memcpy(frameCache+cacheUsed, data, toCopy);
  	cacheUsed+=toCopy; 
}

#define	TIME_SPAN 600

void StreamClientState::writeFile() 
{
  char tmp[100];
  int dir_n;
  
  if (fid == NULL) {
    if (lastTime.tv_sec==0){
    	gettimeofday(&lastTime, (struct timezone *)NULL);
    }
    
    /* open new file - mkdir if needed */  
    dir_n = lastTime.tv_sec/TIME_SPAN;
    if (last_dir_n!=dir_n){
	last_dir_n=dir_n;
	sprintf(tmp, "%s/%s/%d", k_path, k_camid, dir_n);
	if(opendir(tmp)==NULL) mkdir(tmp, 0777);
    }

    sprintf(tmp, "%s/%s/%d/%lu.%04lu", k_path, k_camid,	dir_n, lastTime.tv_sec, lastTime.tv_usec/100);

#ifdef DEBUG
    fprintf(stderr, "new file \"%s\"\n", tmp);
#endif

    fid = fopen(tmp, "wb");
    if (fid == NULL) {
    	printf("unable to open file \"%s\"\n", tmp);
    }
  }

  // Write to our file:
#ifdef TEST_LOSS
  static unsigned const framesPerPacket = 10;
  static unsigned const frameCount = 0;
  static Boolean const packetIsLost;
  if ((frameCount++)%framesPerPacket == 0) {
    packetIsLost = (our_random()%10 == 0); // simulate 10% packet loss #####
  }

  if (!packetIsLost)
#endif
  if (fid != NULL && cacheUsed != 0) {
#ifdef DEBUG
	fprintf(stderr, "writeFile(): %u bytes\n", cacheUsed);
#endif
	fwrite(frameCache, 1, cacheUsed, fid);
  }
  else{
	printf("writeFile(): fid is NULL, %u bytes omitted.\n", cacheUsed);
  }
  cacheUsed=0;
}

void StreamClientState::closeFile()
{
	if (fid != NULL){
		fclose(fid);
		fid = NULL;
#ifdef DEBUG		
		fprintf(stderr, "closeFile(): close file.\n");
#endif
	}
}

// Implementation of "DummySink":

DummySink* DummySink::createNew(UsageEnvironment& env, StreamClientState& scs, char const* streamId) {
  return new DummySink(env, scs, streamId);
}

DummySink::DummySink(UsageEnvironment& env, StreamClientState& scs, char const* streamId)
  : MediaSink(env),
    fSubsession(*scs.subsession),
    fscs(scs) {
  fStreamId = strDup(streamId);
  fReceiveBuffer = new u_int8_t[DUMMY_SINK_RECEIVE_BUFFER_SIZE]; 
  frameCounter=0;
}

DummySink::~DummySink() {
  delete[] fStreamId;
  delete[] fReceiveBuffer;
}

/*
	1. 共用一个文件fid，（共用数据均在ourRTSPClient的scs里）
	2. 共用时间计数，lastTime，公用同一文件中帧计数,frameCounter
	3. i帧出现时，关闭上一个文件，使用lastTime建立新文件，文件帧计数frameCounter清零
	4. 96帧(264)前缀00 00 00 01，08帧(PCMA)前缀00 00 00 01，112帧(DATA)前缀00 00 00 01
	5. 缓存收到的frame，直到见到（video frame的）marker才写入文件

	判断i帧的方法：
		1. 缓存里未发现video_frame_marker的过往帧(包括audio, DATA)
		2. 发现marker后，如果过往video frame数>=3, 则收到一个i帧
		3. 海康：  i帧有5个frame, p帧有2个frame
		   普顺达：i帧有3个frame, p帧有1个frame
	
	数据文件格式:(文件名为timestamp)
	<start_code> 4 bytes
	<frame_data> n bytes
	....
	<start_code> 4 bytes
	<frame_data> n bytes	
*/

void DummySink::afterGettingFrame(void* clientData, unsigned frameSize, unsigned numTruncatedBytes,
				  struct timeval presentationTime, unsigned durationInMicroseconds) {
  DummySink* sink = (DummySink*)clientData;
  sink->afterGettingFrame(frameSize, numTruncatedBytes, presentationTime, durationInMicroseconds);
}


void DummySink::afterGettingFrame(uint32_t frameSize, unsigned numTruncatedBytes,
				  struct timeval presentationTime, unsigned /*durationInMicroseconds*/) {
  unsigned char const start_code[4]   = {0x00, 0x00, 0x00, 0x01};
  
//---------------- print out information about frame ----------------------------
#ifdef DEBUG_PRINT_EACH_RECEIVED_FRAME
  if (fStreamId != NULL) envir() << "channel " << fSubsession.rtpChannelId << "; ";
  envir() << fSubsession.mediumName() << "/" << fSubsession.codecName() << ":\t" << frameSize << " bytes";
  if (numTruncatedBytes > 0) envir() << " (" << numTruncatedBytes << " bytes truncated)";
  char uSecsStr[6+1]; // used to output the 'microseconds' part of the presentation time
  sprintf(uSecsStr, "%06u", (unsigned)presentationTime.tv_usec);
  envir() << ".\ttime: " << (int)presentationTime.tv_sec << "." << uSecsStr;
  if (fSubsession.rtpSource() != NULL && !fSubsession.rtpSource()->hasBeenSynchronizedUsingRTCP()) {
    envir() << "!"; // mark the debugging output to indicate that this presentation time is not RTCP-synchronized
  }
  if (fSubsession.rtpSource()->curPacketMarkerBit()) {
    envir() << "*"; // MarkerBit set
  }
#ifdef DEBUG_PRINT_NPT
  envir() << "\tNPT: " << fSubsession.getNormalPlayTime(presentationTime);
#endif
  envir() << "\n";
#endif
//-------------------------------------------------------------------------------

  if (numTruncatedBytes > 0) {
    envir() << "DummySink::afterGettingFrame(): The input frame data was too large for our buffer size .  "
            << numTruncatedBytes << " bytes of trailing data was dropped!  Need more buffer at least "
            << DUMMY_SINK_RECEIVE_BUFFER_SIZE + numTruncatedBytes << "\n";
  }

  if (fSubsession.rtpSource()->rtpPayloadFormat()==0x70 && frameSize>90){
	// 112帧 size=96,过滤掉
  }else {
	fscs.addCache(start_code, 4);
	fscs.addCache(fReceiveBuffer, frameSize);
  }

  if (fSubsession.rtpSource()->rtpPayloadFormat()==0x60){ // 96帧，video/H264
  	frameCounter++;
  	if (fSubsession.rtpSource()->curPacketMarkerBit()){
  		if (frameCounter>=3){
  			// 收到 i-frame，关闭上一个文件
  			fscs.closeFile();
  		}
  		// 写入数据到文件
  		fscs.writeFile();
/*  		
		if (fscs.fid == NULL || fflush(fscs.fid) == EOF) {
		    // The output file has closed.  Handle this the same way as if the input source had closed:
		    if (fSource != NULL) fSource->stopGettingFrames();
		    onSourceClosure();
		    return;
		}
*/		
		frameCounter=0; 
  	}
  	fscs.lastTime = presentationTime;
  }

  // Then continue, to request the next frame of data:
  continuePlaying();
}

Boolean DummySink::continuePlaying() {
  if (fSource == NULL) return False; // sanity check (should not happen)

  // Request the next frame of data from our input source.  "afterGettingFrame()" will get called later, when it arrives:
  fSource->getNextFrame(fReceiveBuffer, DUMMY_SINK_RECEIVE_BUFFER_SIZE,
                        afterGettingFrame, this,
                        onSourceClosure, this);
  return True;
}

