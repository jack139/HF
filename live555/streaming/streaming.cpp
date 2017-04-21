// Copyright (c) 1996-2014, Live Networks, Inc.  All rights reserved
// LIVE555 Media Server
// main program

#include <time.h>
#include <BasicUsageEnvironment.hh>
#include "DynamicRTSPServer.hh"

#define SNAP_STORE_PATH "."

extern char *snap_store_path;  // equal to k_path, used and defined by ByteStreamMultiFileSource

char g_tmp[200];
char snap_path[200];

char *date_str(void)
{
	time_t now;
	struct tm *tm_now;
 
	time(&now);
	tm_now = localtime(&now);
	
	strftime(g_tmp,	100, "%a, %b %d %Y %H:%M:%S %Z", tm_now);

	return g_tmp;
}

int main(int argc, char** argv) 
{
  char  pxy_service[10];	/* port	number of proxy	 */ 
  
  // Begin by setting up our usage environment:
  TaskScheduler* scheduler = BasicTaskScheduler::createNew();
  UsageEnvironment* env = BasicUsageEnvironment::createNew(*scheduler);

  UserAuthenticationDatabase* authDB = NULL;
#ifdef ACCESS_CONTROL
  // To implement client access control to the RTSP server, do the following:
  authDB = new UserAuthenticationDatabase;
  authDB->addUserRecord("username1", "password1"); // replace these with real strings
  // Repeat the above with each <username>, <password> that you wish to allow
  // access to the server.
#endif

  printf("\nStreaming Server 2.0  ----  stream server for H.264 Kam (library from live555.com)\n");
  printf("written by jack139, F8 Network 2014 (%s)\n\n", date_str());

  switch (argc) {
  case  3:
	strcpy(pxy_service, argv[1]);
	strcpy(snap_path, argv[2]);
	break;
  default:
	printf("usage: streaming <service port> <snap_path>\n");
	exit(1);
  }

  snap_store_path=snap_path;
  
  // Create the RTSP server.  Try first with the default port number (554),
  // and then with the alternative port number (8554):
  RTSPServer* rtspServer;
  portNumBits rtspServerPortNum = atoi(pxy_service);  // u_int16_t
  rtspServer = DynamicRTSPServer::createNew(*env, rtspServerPortNum, authDB);
  if (rtspServer == NULL) {
    *env << "Failed to create RTSP server: " << env->getResultMsg() << "\n";
    exit(1);
  }

  printf("Listen on port %u ...\n\n", rtspServerPortNum);
  
  env->taskScheduler().doEventLoop(); // does not return

  return 0; // only to prevent compiler warning
}
