/* 
 * RTSP	recorder
 * 
 * F8 Network (c) 2014,	jack139
 *
 * Version: 1.0
 *
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <stdint.h>
#include <dirent.h>

#include "vaap264.h"
#include "http_lib.h"
#include "kam.h"

#define	TIME_SPAN 600

#define	RTSP_PORT 554
#define	RTSP_INTERLEAVED_SIZE 4
#define	RTSP_HEADER_SIZE 8

uint32_t max_size=0;
int queue_pkt_num, last_dir_n;
uint32_t queue_pkt_len;
uint32_t out_size, in_size;
static char *outbuff, *inbuff;
static enum STATE snap_state, next_state, next_step;
char rtsp_session[50]={'\0'};

void prepare_options(void)
{
	sprintf(outbuff, 
		"OPTIONS rtsp://%s:%d/PSIA/streaming/channels/101 RTSP/1.0\r\n"
		"CSeq: 2\r\n"
		"User-Agent: NKPlayer-1.00.00.081112\r\n"
		"\r\n", k_kamip, RTSP_PORT);

	//puts(outbuff);
  
	out_size = strlen(outbuff);
}

void prepare_describe(char *auth)
{
	sprintf(outbuff, 
		"DESCRIBE rtsp://%s:%d/PSIA/streaming/channels/101 RTSP/1.0\r\n"
		"CSeq: 3\r\n"
		"Authorization: Basic %s\r\n"
		"Accept: application/sdp\r\n"
		"User-Agent: NKPlayer-1.00.00.081112\r\n"
		"\r\n", k_kamip, RTSP_PORT, auth);

	//puts(outbuff);
  
	out_size = strlen(outbuff);
}

void prepare_setup(char	*auth)
{
	sprintf(outbuff, 
		"SETUP rtsp://%s:%d/PSIA/streaming/channels/101/trackID=1 RTSP/1.0\r\n"
		"CSeq: 4\r\n"
		"Authorization: Basic %s\r\n"
		"Transport: RTP/AVP/TCP;unicast;interleaved=0-1;ssrc=0\r\n"
		"User-Agent: NKPlayer-1.00.00.081112\r\n"
		"\r\n", k_kamip, RTSP_PORT, auth);

	//puts(outbuff);
  
	out_size = strlen(outbuff);
}

void prepare_play(char *auth, char* session)
{
	sprintf(outbuff, 
		"PLAY rtsp://%s:%d/PSIA/streaming/channels/101 RTSP/1.0\r\n"
		"CSeq: 5\r\n"
		"Authorization: Basic %s\r\n"
		"Session: %s\r\n"
		"Rate-Control: yes\r\n"
		"Scale: 1.000\r\n"
		"User-Agent: NKPlayer-1.00.00.081112\r\n"
		"\r\n", k_kamip, RTSP_PORT,	auth, session);

	//puts(outbuff);
  
	out_size = strlen(outbuff);
}

void prepare_heartbeat(char *auth, char* session)
{
	sprintf(outbuff, 
		"HEARTBEAT rtsp://%s:%d/PSIA/streaming/channels/101 RTSP/1.0\r\n"
		"CSeq: 6\r\n"
		"Authorization: Basic %s\r\n"
		"Session: %s\r\n"
		"User-Agent: NKPlayer-1.00.00.081112\r\n"
		"\r\n", k_kamip, RTSP_PORT,	auth, session);

	//puts(outbuff);
  
	out_size = strlen(outbuff);
}

/* The connect routine including the command to	set the	socket non-blocking.  */
int doconnect(char *address, int port)	
{  
  int s; 
  struct hostent *hp;
  struct sockaddr_in server;

  /* resolve hostname */
  if ((hp = gethostbyname( address ))) {
    memset((char *) &server,0, sizeof(server));
    memmove((char *) &server.sin_addr, hp->h_addr, hp->h_length);
    server.sin_family =	hp->h_addrtype;
    server.sin_port = (unsigned	short) htons( port );
  } else
    return -1;

  /* create socket */
  if ((s = socket(AF_INET, SOCK_STREAM,	0)) < 0) return	-2;
  setsockopt(s,	SOL_SOCKET, SO_KEEPALIVE, 0, 0);
  
  /* socket connect */
  if (conn_nonb(s, &server, sizeof(server), 5, 1) < 0) return -3;
  else return(s);  
}  


unsigned int search_next_pkt(char *buf,	uint32_t size)
{
	uint32_t i;
	char *p;
	struct RTSP_interleaved	*c;
	
	p=buf;
	i=0;
	
	while(i<size){
		for (; *p!='$' && i<size; p++, i++);
		if ((size-i)>RTSP_INTERLEAVED_SIZE){
			c=(struct RTSP_interleaved *)p;
			if (c->rtp_head.version==2 && (c->rtp_head.pt==96 || c->rtp_head.pt==112))
				return i;
			else{
				p++;
				i++;
			}
		}
	}
	return size;
}

void *snap_thread(void)
{   
  fd_set read_flags, write_flags;
  struct timeval waitd;
  int thefd, filefd;
  int err, rec_count=0,	err_count=0;
  //int	connID=-1;
  long long jpg_time=0LL;
  uint32_t last_stamp=0;
  time_t heartbeat;
  char tmp[100]={'\0'};

  puts("snap: thread started.");
  
  outbuff=query_header[P_SNAP];
  inbuff=cache_buf;
  
  thefd=-1;
  filefd=-1;
  snap_state=IDLE;
  time(&heartbeat);

  while(1){
    fflush(stdout);

    if (err_count>10) break; /*	exit snap_thread will cause reboot camera */

    if (rec_count>20){
      puts("snap: rec_count reach MAX. re-connect ...");
      snap_state=CONNECT;
      err_count++;
    }
    
    switch(snap_state){
    case CONNECT:
      puts("1--	CONNECT");
      if (thefd>0) { close(thefd); thefd=-1; }
      if (filefd>0) { close(filefd); filefd=-1;	}
      memset(outbuff,0,HEADER_SIZE); 
      out_size=0;
      in_size=0;
      rec_count=0;
      last_dir_n=0;
      queue_pkt_num=0;
      queue_pkt_len=0;

      thefd=doconnect(k_kamip, RTSP_PORT); 
      if(thefd<0) {  
	printf("snap: 1	could not connect to camera. errno=%d\n", thefd);  
	err_count++;
      }
      else{
	snap_state=OPTIONS;
      }
      break;

    case OPTIONS:
      puts("1--	OPTIONS");
      prepare_options();
      snap_state=SENDING;
      next_state=WAIT_RESP;
      next_step=DESCRIBE;
      break;
      
    case DESCRIBE:
      puts("1--	DESCRIBE");
      prepare_describe(k_auth);
      snap_state=SENDING;
      next_state=WAIT_RESP;
      next_step=SETUP;
      break;

    case SETUP:
      puts("1--	SETUP");
      prepare_setup(k_auth);
      snap_state=SENDING;
      next_state=WAIT_RESP;
      next_step=PLAY;
      break;

    case PLAY:
      puts("1--	PLAY");
      prepare_play(k_auth, rtsp_session);
      snap_state=SENDING;
      next_state=WAIT_RESP;
      next_step=RECORDING;
      heartbeat=time(NULL); /* send HEARTBEAT in 10 seconds */
      break;

    case CLOSE:
      puts("1--	CLOSE"); 
      close(thefd);
      close(filefd);
      thefd=-1;
      filefd=-1;
      out_size=0;
      in_size=0;
      rec_count=0;
      last_dir_n=0;
      queue_pkt_num=0;
      queue_pkt_len=0;
      snap_state=IDLE;
      rec_count=0;
      break;
    
    case IDLE:
      //puts("1-- IDLE");
      
      //k_delay=1;  /* only for	test */
      
      if (k_delay>0){ /* start to recorde video	*/
	snap_state=CONNECT;
      }
      else{
	sleep(1);
      }
      break;
    
    case RECORDING:
      
#ifdef DEBUG_VAAP
      puts("1--	RECORDING");
#endif
      if (k_delay<0){
	snap_state=CLOSE;
      }
      else{
	if (time(NULL)-heartbeat>4) snap_state=HEARTBEAT;
      }
      break;

    case HEARTBEAT:
      //puts("1-- HEARTBEAT");
      prepare_heartbeat(k_auth,	rtsp_session);
      snap_state=SENDING;
      next_state=RECORDING;
      time(&heartbeat);
      break;

    case SENDING:
      rec_count++;
      printf("1-- SENDING, rec_count=%d\n", rec_count);
      break;

    case WAIT_RESP:
      rec_count++;
      printf("1-- WAIT_RESP, rec_count=%d\n", rec_count);
      break;
      
    default:
      printf("snap: 1 unkonwn state! snap_state=%d\n", snap_state);
      break;
    }


    if (in_size>=4){
	uint16_t lg;
	
	max_size=(max_size>in_size)?max_size:in_size;
	  
	if (inbuff[0]=='R' && inbuff[1]=='T' &&	inbuff[2]=='S' && inbuff[3]=='P'){
	  char *p;

	  if (strstr(inbuff, "200 OK")==NULL){
	      printf("\n!!! response ERROR\n");
	      puts(inbuff);
	      next_state=CLOSE;
	      in_size=0;
	  }
	  else if (snap_state==RECORDING){
	    /* 
	     * process response	of HEARTBEAT
	     */
	    p=strstr(inbuff, "\r\n\r\n");
	    if (p==NULL){
	      printf("\n!!! HEARTBEAT response format error\n");
	      next_state=CLOSE;
	      in_size=0;
	    }
	    else{
	      *(p+2)='\0';
	      lg=(uint16_t)(p-inbuff)+4;
	      //puts(inbuff);

	      /* remove	from cache/inbuff */
	      in_size-=lg;
	      if ((in_size)!=0)	memcpy(inbuff, inbuff+lg, in_size);
	    }		     
	  }
	  else if (next_step==PLAY){ 
	    /* 
	     * process SETUP response, get session string
	     */
	    char *se;
	    
	    /*puts(inbuff);*/
	    
	    p=strstr(inbuff, "Session:");
	    if (!p){
	      printf("\n!!! Session NOT	FOUND in SETUP response\n");
	      next_state=CLOSE;
	    }
	    else{
	      p+=8;
	      for (; *p==' '; p++);
	      se=p;
	      for (; *p&&*p!=' '&&*p!='\r'&&*p!='\n'; p++);
	      *p='\0';
	      strcpy(rtsp_session, se);
	      snap_state=next_step;
	    }
	    in_size=0;
	  }
	  else{
	    /*
	     * process responses of DECRIBE, PLAY
	     */
	     
	    /* puts(inbuff); */
	    
	    snap_state=next_step;
	    in_size=0;
	  }

	  rec_count=0;
	}
	else if	(inbuff[0]=='$'){	   
	  while(inbuff[0]=='$'){
	    struct RTSP_interleaved *c;
	    int	dir_n;
	    
	    c=(struct RTSP_interleaved *)(inbuff);
	    lg = ntohs(c->payload_len)+RTSP_INTERLEAVED_SIZE;
	    
	    if (in_size>=lg){
#if 0
	      if (c->rtp_head.pt!=96 || c->rtp_head.x)
		printf("ver=%d x=%d seq=%d type=%d in_size=%u lg=%u ts=%u\n", 
			c->rtp_head.version, c->rtp_head.x, ntohs(c->rtp_head.seqno), 
			c->rtp_head.pt,	in_size, lg, ntohl(c->rtp_head.ts));

	      if (c->rtp_head.x){ /* check RTP header extension	*/
		struct RTP_extension *c2;
		uint32_t *p;
		int i;
		
		c2=(struct RTP_extension *)(&(c->rtp_head)+1);
		printf("cust_def=%X len=%d\n", ntohs(c2->cust_def), ntohs(c2->length));
		
		p=(uint32_t *)(c2+1);
		for (i=0; i<ntohs(c2->length); i++)
		  printf(" %08X\n", ntohl(p[i]));
	      }
#endif

	      if (c->rtp_head.pt==112 && lg==68){
		/*
		  type=112 lg=68
		  type=112 lg=36
		  1st main frame
		  type=112 lg=68
		  type=112 lg=36
		  2nd main frame
		  .
		  .
		  .
		  type=112 lg=68
		  type=112 lg=36
		  (N)th	main frame
		*/

		/* make	snapshot time */
		if (last_stamp==0){
			jpg_time=milli_time();
			puts("snap: NEW timestamp");
		}
		else{
			int32_t diff=ntohl(c->rtp_head.ts)-last_stamp;
			
			if (diff<0 || diff>1800000){ /* 20sec = 1800,000/90,000 */
				/*
				 * timestamp has beeb reset
				 * diff between packets' timestamp should be less than 20 sec
				 */
				jpg_time=milli_time();
				printf("snap: RESET timestamp %d %u %u\n", diff, ntohl(c->rtp_head.ts), last_stamp);
			}
			else{
				/*
				 * diff(in sec) = (timestamp2 - timestamp1)/90000
				 */
				jpg_time+=(long long)diff*100LL/9;
			}
		}
		last_stamp=ntohl(c->rtp_head.ts);
#ifdef DEBUG_VAAP
		printf("%u %lld	", ntohl(c->rtp_head.ts), jpg_time);
#endif

		/* close last file */
		if (filefd>0){
		  close(filefd);
#if 0
		  printf("max_size=%u num=%u len=%u next_fn=%s\n", 
		  		max_size, queue_pkt_num, queue_pkt_len, tmp);
#endif
		}

		/* open	new file - mkdir if needed */  
		dir_n =	jpg_time/1000000/TIME_SPAN;
		if (last_dir_n!=dir_n){
		  last_dir_n=dir_n;
		  sprintf(tmp, "%s/%s/%d", k_path, k_camid, dir_n);
		  if(opendir(tmp)==NULL) mkdir(tmp, 0777);
		}
		/*
		 *  like this "/data2/snap_store/530309d585cb310609017c03/2324360/1394616229.4363"
		 */
		sprintf(tmp, "%s/%s/%d/%.04f", k_path, k_camid,	dir_n, jpg_time/1000000.0);
		filefd=open(tmp, O_WRONLY|O_CREAT, 0666);
		if (filefd==-1){
		  printf("FAIL to open local file: %s\n", tmp);
		}

#if 0
		printf("open new file: %s\n", tmp);
#endif

		/* reset counter */
		queue_pkt_num=1;
		queue_pkt_len=lg;
	      }
	      else{
		queue_pkt_num++;
		queue_pkt_len+=lg;
	      }

	      /* write to file */
	      if (filefd>0){
		write(filefd, (char *)c, lg);
	      }

	      /* remove	from cache/inbuff */
	      in_size-=lg;
	      if ((in_size)!=0)	memcpy(inbuff, inbuff+lg, in_size);

	      rec_count=0;
	    }
	    else
	      break;
	  }
	}
	else{
	  unsigned int i;
	  
	  printf("snap:	unknown	packet header! in_size=%u\n", in_size);
	  for (i=0;i<8;i++) printf("%02X ", (unsigned char)(*(inbuff+i)));
	  printf("\n");
	  
	  /* find next packet in in_buff */
	  i=search_next_pkt(inbuff, in_size);
	  in_size-=i;
	  if ((in_size)!=0) memcpy(inbuff, inbuff+i, in_size);
	  printf("snap:	  %u bytes ignored\n", i);

	  last_stamp=0;	/* reset time_counter */
	}
    }

    
    /* no connection exist mean	in IDLE	state */
    if (thefd<0) continue;
    
    waitd.tv_sec = 1;	  /* Make select wait up to 1 second for data  */
    waitd.tv_usec = 0;	  
    FD_ZERO(&read_flags); /* Zero the flags ready for using  */
    FD_ZERO(&write_flags);  
    if (thefd>0) FD_SET(thefd, &read_flags);  
    if (out_size>0) FD_SET(thefd, &write_flags);
    err=select(thefd+1,&read_flags,&write_flags,(fd_set	*)0,&waitd);
    if(err<0) continue;
    
    if(FD_ISSET(thefd, &read_flags)) { /* Socket ready for reading  */
      FD_CLR(thefd, &read_flags);  
      err =  read(thefd, inbuff+in_size, CACHE_BUF_SIZE-in_size-1);
      if ( err<0 ) {  
	if (errno==EAGAIN)
	  puts("snap: no data available.");
	else{
	  printf("snap:	read fail - err	= %d errno = %d	\n", err, errno);
	  snap_state=CLOSE;
	  continue;
	}
      }	 
      else if (err>0){
	  in_size+=err;
	  inbuff[in_size]='\0';

#ifdef DEBUG_VAAP
	  printf("snap:	read (%d) bytes, in_size=%u\n",	err, in_size);
#endif
      }
    }

    if(FD_ISSET(thefd, &write_flags)) {	/* Socket ready	for writing  */
      FD_CLR(thefd, &write_flags);  
      err = write(thefd,outbuff,out_size);
      if ( err<0 ) {  
	if (errno==EAGAIN)
	  puts("snap: write NOT	available.");
	else{
	  printf("snap:	write fail: err	= %d errno = %d	\n", err, errno);
	  snap_state=CLOSE;
	  continue;
	}
      }	 
      else{
#ifdef DEBUG_VAAP
	printf("snap: write (%d) bytes\n", err);
#endif
	/* supposed that all data has been sent, because our packet is so small	*/
	memset(outbuff,0,out_size);  
	out_size=0;
	snap_state=next_state;
      }
    }


    /* now the loop repeats over again	*/
  }  
 
  if (thefd>0) close(thefd); 
  if (filefd>0)	close(filefd); 

  puts("snap: thread exit.");  
  
  return NULL;
}
