// Copyright (c) 1996-2014, Live Networks, Inc.  All rights reserved
// A subclass of "RTSPServer" that creates "ServerMediaSession"s on demand,
// based on whether or not the specified stream name exists as a file
// Implementation

#include "DynamicRTSPServer.hh"
#include <liveMedia.hh>
#include <string.h>

DynamicRTSPServer*
DynamicRTSPServer::createNew(UsageEnvironment& env, Port ourPort,
			     UserAuthenticationDatabase* authDatabase,
			     unsigned reclamationTestSeconds) {
  int ourSocket = setUpOurSocket(env, ourPort);
  if (ourSocket == -1) return NULL;

  return new DynamicRTSPServer(env, ourSocket, ourPort, authDatabase, reclamationTestSeconds);
}

DynamicRTSPServer::DynamicRTSPServer(UsageEnvironment& env, int ourSocket,
				     Port ourPort,
				     UserAuthenticationDatabase* authDatabase, unsigned reclamationTestSeconds)
  : RTSPServer(env, ourSocket, ourPort, authDatabase, reclamationTestSeconds) {
}

DynamicRTSPServer::~DynamicRTSPServer() {
}

static ServerMediaSession* createNewSMS(UsageEnvironment& env, char const* fileName); // forward

ServerMediaSession* DynamicRTSPServer::lookupServerMediaSession(char const* streamName) 
{
	// Next, check whether we already have a "ServerMediaSession" for this file:
	ServerMediaSession* sms = RTSPServer::lookupServerMediaSession(streamName);
	Boolean smsExists = sms != NULL;

	// Handle the four possibilities for "fileExists" and "smsExists":
	if (!smsExists) {
		// Create a new "ServerMediaSession" object for streaming from the named file.
		sms = createNewSMS(envir(), streamName);
		addServerMediaSession(sms);
	}

	return sms;
  
}


#define NEW_SMS(description) do {\
char const* descStr = description\
    ", streamed by the LIVE555 Media Server";\
sms = ServerMediaSession::createNew(env, fileName, fileName, descStr);\
} while(0)

static ServerMediaSession* createNewSMS(UsageEnvironment& env, char const* fileName) 
{
  ServerMediaSession* sms = NULL;
  Boolean const reuseSource = False;

  env << "Assumed to be a H.264 Video Elementary Stream file: " << fileName << "\n";
  NEW_SMS("H.264 Video");
  OutPacketBuffer::maxSize = 500000; // allow for some possibly large H.264 frames
  sms->addSubsession(H264VideoFileServerMediaSubsession::createNew(env, fileName, reuseSource));
  
  return sms;
}
