/* 
 * RTSP	stream server
 * 
 * F8 Network (c) 2014,	jack139
 *
 * Version: 1.0
 *
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <dirent.h>
#include <errno.h>

#include "streaming.h"
#include "vaap264.h"

#define	NAMLEN(dirent) strlen((dirent)->d_name)

#define	TIME_SPAN 600
#define	FN_LIST_SIZE 500
#define	BUF2_SIZE 1024
#define	SOCK_SIZE 1464

/*
char test_ts[]="1394939030";
char test_camid[]="530309d585cb310609017c03";
char test_hid[]="TEST";
*/

/* DESCRIBE */
char re0[]="RTSP/1.0 200 OK\r\n"
	"CSeq: %d\r\n"
	"Content-Type: application/sdp\r\n"
	"Content-Length: %d\r\n"  /* old = 438 */
	"\r\n%s";
	
char sdp[]="v=0\r\n"
	"o=- %lld %lld IN IP4 192.168.100.250\r\n"
	"s=Media Presentation\r\n"
	"e=NONE\r\n"
	"b=AS:5050\r\n"
	"t=0 0\r\n"
	"a=control:*\r\n"
	"m=video 0 RTP/AVP 96\r\n"
	"b=AS:5000\r\n"
	"a=control:trackID=1\r\n"
	"a=rtpmap:96 H264/90000\r\n"
/*	"a=fmtp:96 profile-level-id=420029; packetization-mode=1; sprop-parameter-sets=Z00AH5pmAoAt/zUBAQFAAAD6AAAdTAE=,aO48gA==\r\n" */
	"a=fmtp:96 profile-level-id=420029; packetization-mode=1; sprop-parameter-sets=Z00AHpWoLASZ,aO48gA==\r\n"
	"a=Media_header:MEDIAINFO=494D4B48010100000400010000000000000000000000000000000000000000000000000000000000;\r\n"
	"a=appversion:1.0\r\n";

/* SETUP */
char re1[]="RTSP/1.0 200 OK\r\n"
	"CSeq: %d\r\n"
	"Session: 1200060553;timeout=60\r\n"
	"Transport: RTP/AVP/TCP;unicast;interleaved=0-1;ssrc=6bfb94f3;mode=\"play\"\r\n"
	"Date:  %s\r\n"
	"\r\n";

/* PLAY	*/
char re2[]="RTSP/1.0 200 OK\r\n"
	"CSeq: %d\r\n"
	"Session: 1200060553\r\n"
	"Scale: 1.0\r\n"
	"RTP-Info: url=trackID=1;seq=102\r\n"
	"Date:  %s\r\n"
	"\r\n";

/* HEARTBEAT */
char re3[]="RTSP/1.0 200 OK\r\n"
	"CSeq: %d\r\n"
	"Date:  %s\r\n"
	"\r\n";

char snap_path[200];
unsigned int childs=0;
char buf[1500+10];  /* maximum MTU size	is 1500	*/
char inbuf[1500+10];  /* maximum MTU size is 1500 */
char outbuf[1500+10];  /* maximum MTU size is 1500 */
char g_tmp[200];

struct req_info{
	char *camid;
	int camid_len;
	char *ts;
	int ts_len;
	char *hid;
	int hid_len;
};

struct snap_fn{
	char fn[20];
	char dn[10];
};

char *date_str(void)
{
	time_t now;
	struct tm *tm_now;
 
	time(&now);
	tm_now = localtime(&now);
	
	strftime(g_tmp,	100, "%a, %b %d %Y %H:%M:%S %Z", tm_now);

	return g_tmp;
}

long long milli_time(void){
  struct timeval t2;

  if (gettimeofday(&t2, (struct timezone *)NULL) == 0)
    return (long long)t2.tv_sec * 1000000LL + (long long)t2.tv_usec;
  else
    return 0;
}

/* check hid */
const char codebook[]="EPLKJHQWEIRUSDOPCNZX";
int check_hid(char *ts,	char *hid)
{
	unsigned short i;
  
	for(i=0; i<strlen(ts); i++)
		if (codebook[ts[i]%20]!=hid[i])	return -1;
      
	return 0;
}

/* compare function for	qsort()	*/
static int fn_cmp (const void *c1, const void *c2)
{
	return strcmp (((struct	snap_fn	*)c1)->fn, ((struct snap_fn *)c2)->fn);
}

/* get filename	list */
int fc(char *path, long	st_i, long et_i, struct	snap_fn	*list)
{
	DIR *dp,*dp2;
	struct dirent *ep, *ep2;
	char tmp[100];
	int total=0L, st_d, et_d, this_d;
	long this_i;

	st_d = st_i/TIME_SPAN;
	et_d = et_i/TIME_SPAN;
  
	dp = opendir(path);
	if (dp != NULL){
		while ((ep = readdir(dp))){
			if (ep->d_name[0]=='.' && 
				(NAMLEN(ep)==1 ||(ep->d_name[1]=='.' &&	NAMLEN(ep)==2))) continue;
			this_d=atoi(ep->d_name);
			if (this_d<st_d	|| this_d>et_d)	continue;
			sprintf(tmp, "%s/%s", path, ep->d_name);
			dp2 = opendir (tmp);
			if (dp2	!= NULL){
				while ((ep2 = readdir(dp2))){
					if (total==FN_LIST_SIZE) break;
					if (ep2->d_name[0]=='.'	&& 
						(NAMLEN(ep2)==1	||
							(ep2->d_name[1]=='.' &&	NAMLEN(ep2)==2))) continue;
					this_i=atol(ep2->d_name);
					if (this_i<st_i	|| this_i>et_i)	continue;
					strcpy(list[total].fn, ep2->d_name);
					strcpy(list[total].dn, ep->d_name);
					/* printf("%u: %s/%s\n", childs, list[total].dn, list[total].fn); */
					total++;
				}
				(void) closedir	(dp2);
			}
		}
		(void) closedir	(dp);
	}
  
  return total;
}

/* get CSeq in request header */
int get_cseq(char *req){
	char *p, *p2, t;
	int seq;
	
	p=strstr(req, "CSeq");
	if (!p){
		printf("%u: ! CSeq NOT FOUND\n", childs);
		return -1;
	}
	
	/* get CSeq */
	p+=5;
	p2=p;
	for (; *p&&*p!='\r'&&*p!='\n'; p++);
	if (!*p){
		printf("%u: ! find CSeq	encounter error\n", childs);
		return -1;
	}
	t=*p;
	*p='\0';
	seq=atoi(p2);
	*p=t;
/*
	printf("%u: Cseq=%s %d\n", childs, p2, atoi(p2));
*/
	return seq;
}
/*------------------------------------------------------------------------
 * livesrem - process livestream request
 *------------------------------------------------------------------------
 */
int livestream(int fd, char *req){
	fd_set read_flags, write_flags;
	struct timeval waitd;
	struct timespec	delay;
	int fd2, n, n2,	next, lg, i, fn_num, bye;
	long st_i, et_i;
	struct RTSP_interleaved	*c;
	struct req_info	live_req;
	char *buf2, *p;
	struct snap_fn *fn_list;
	char cam_path[100];
	uint16_t seqno;
	time_t heartbeat;
	uint32_t ssrc_save=0;

	sprintf(outbuf,	re2, get_cseq(buf), date_str());
/*	  
	printf("%u: >>>\n", childs);
	printf("%s\n", outbuf);	
*/
	write(fd, outbuf, strlen(outbuf));
  
	buf2=malloc(1024*BUF2_SIZE);
	if (!buf2){
		printf("%u: ! fail to alloc memory\n", childs);
		return 0;
	}
      
	fn_list=malloc(sizeof(struct snap_fn)*FN_LIST_SIZE);
	if (!fn_list){
		printf("%u: ! fail to alloc memory\n", childs);
		free(buf2);
		return 0;
	}

	/* get parameters */
	p=strstr(req, "Basic");
	if (!p){
		printf("%u: ! request header error\n", childs);
		goto exit;
	}
	
	/* get camid */
	p+=6;
	live_req.camid=p;
	for (; *p&&*p!=' '; p++);
	if (!*p){
		printf("%u: ! request header error, camid\n", childs);
		goto exit;
	}
	live_req.camid_len=(int)(p-live_req.camid);
	*p='\0';
	
	/* get start tick */
	p++;
	live_req.ts=p;
	for (; *p&&*p!=' '; p++);
	if (!*p){
		printf("%u: ! request header error, tick\n", childs);
		goto exit;
	}
	live_req.ts_len=(int)(p-live_req.ts);
	*p='\0';
      
	/* get start hid */
	p++;
	live_req.hid=p;
	for (; *p&&*p!=' '&&*p!='\r'&&*p!='\n';	p++);
	if (!*p){
		printf("%u: ! request header error, hid\n", childs);
		goto exit;
	}
	live_req.hid_len=(int)(p-live_req.hid);
	*p='\0';
	
	/* check hid */
/*	  
	if (check_hid(live_req.ts, live_req.hid)<0){
		printf("%u: ! wrong hid	value\n", childs);
		goto exit;
	}
*/
	
	/* ============= only for test ================== 
	live_req.ts=test_ts;
	live_req.camid=test_camid;
	live_req.hid=test_hid;
	 ----------------------------------------------	*/

	/* find	filenames */
	st_i = atol(live_req.ts);
	et_i = st_i+1800; /* 30 mins */
	  
	sprintf(cam_path, "%s/%s", snap_path, live_req.camid);
	
	fn_num=fc(cam_path, st_i, et_i,	fn_list);
	
	qsort(fn_list, fn_num, sizeof(struct snap_fn), fn_cmp);
	
	printf("%u: [%s] [%s] [%s] [%ld] [%d]\n", 
		childs,	live_req.camid,	live_req.ts, live_req.hid, st_i, fn_num);
	
/*
	for(i=0; i<fn_num; i++) printf("%u: %s/%s\n", childs, fn_list[i].dn, fn_list[i].fn);
*/

	fflush(stdout);
		
	/* ready to play */
	seqno=100;
	delay.tv_sec=0;
	delay.tv_nsec=10000000;
	outbuf[0]='\0'; /* ready to receive HEARTBEAT */
	bye=0;
	time(&heartbeat);
	for (i=0; i<fn_num; i++){
		fflush(stdout);

		/* like "/usr/data3/snap_store/5337d71685cb3115b16c53ce/2326948/1396168854.9327" */
		sprintf(cam_path, "%s/%s/%s/%s", snap_path, live_req.camid, fn_list[i].dn, fn_list[i].fn);
		fd2=open(cam_path, 0);
		n=read(fd2, buf2, 1024*BUF2_SIZE-1);
		if (n==-1){
			printf("%u: read fail! errno=%d path=%s\n", childs, errno, cam_path);
			close(fd2);
			continue;
		}
		close(fd2);
		
/*
		printf("%u: %s\n", childs, cam_path);  
		printf("%u: >>>\n", childs); 
*/
	  
		next=0;
		lg=0;
		while(next<n){
			fflush(stdout);

			if (time(NULL)-heartbeat>60) { /* 60 seconds NO	HEARTBEAT to timeout */
				printf("%u: HEARTBEAT need\n", childs); 
				goto exit;
			}

			waitd.tv_sec = 1;     /* Make select wait up to	1 second for data  */
			waitd.tv_usec =	0;    
			FD_ZERO(&read_flags);
			FD_ZERO(&write_flags);
			FD_SET(fd, &read_flags);  
			FD_SET(fd, &write_flags);
			n2=select(fd+1,&read_flags,&write_flags,(fd_set	*)NULL,&waitd);
			if(n2<0){
				printf("%u: select() fail.", childs);
				nanosleep(&delay, (struct timespec *)NULL);
				continue;
			}
			
			/* 
			 * Socket ready	for reading: HEARTBEAT,	TEARDOWN
			 */
			if(FD_ISSET(fd,	&read_flags)) {	
				FD_CLR(fd, &read_flags);  
				n2 =  read(fd, inbuf, 1500);

				if ( n2<0 ) {  
					if (errno==EAGAIN)
						printf("%u: no data available.", childs);
					else
						printf("%u: read fail. errno = %d \n", childs, errno);
					inbuf[0]='\0';
				}  
				else if	(n2>0){
					inbuf[n2]='\0';
/*
					printf("%u: <<<\n", childs);
					printf("%s\n", inbuf); 
*/
					if (strstr(inbuf, "HEARTBEAT")){
						time(&heartbeat);
						sprintf(outbuf,	re3, get_cseq(inbuf), date_str());
					}
					else if	(strstr(inbuf, "TEARDOWN")){
						bye=1;
						sprintf(outbuf,	re3, get_cseq(inbuf), date_str());
					}
					else{
						printf("%u: Unknown command\n", childs);
						printf("%s\n", inbuf); 
						sprintf(outbuf,	re3, get_cseq(inbuf), date_str());
					}
				}
				else{
					if (bye) goto exit;
				}
			}

			/*
			 *  Socket ready for writing: RTP, RTSP	reply
			 */
			if(FD_ISSET(fd,	&write_flags)) { 
				FD_CLR(fd, &write_flags);  
				
				/*
				 * Write RTSP reply
				 */
				if (outbuf[0] && lg==0){
					n2=write(fd, outbuf, strlen(outbuf));
					if (n2<0){
						if (errno==EAGAIN)
							printf("%u: ! write NOT available\n", childs);
						else
							printf("%u: ! fail to send. error=%d\n", childs, errno);
					}
					else{
/*
						printf("%u: >>>\n", childs);
						printf("%s\n", outbuf);	
*/
						outbuf[0]='\0';
						
						if (bye){
							printf("%u: BYE--BY--TEARDOWN\n", childs);
							goto exit;
						}
					}
					continue;
				}

				/*
				 * Write RTP packet
				 */
				c = (struct RTSP_interleaved *)(buf2+next);
	      
				if (lg==0){
					if (c->dollar!='$'){
						printf("%u: ! wrong packet format\n%d bytes sent\n", 
							childs, next);
						goto exit;
					}
					lg = ntohs(c->payload_len)+4;
					c->rtp_head.seqno = htons(seqno);
					seqno++;
					nanosleep(&delay, (struct timespec *)NULL);
					
					ssrc_save=c->rtp_head.ssrc; /* save for BYE rtcp */

#if 0					
					if (c->rtp_head.pt!=96 || c->rtp_head.x)
						printf("ver=%d x=%d seq=%d type=%d lg=%u ts=%u\n", 
							c->rtp_head.version, c->rtp_head.x, ntohs(c->rtp_head.seqno), 
							c->rtp_head.pt,	lg, ntohl(c->rtp_head.ts));
#endif
				}
				n2=write(fd, buf2+next, (lg>SOCK_SIZE)?SOCK_SIZE:lg);
				if (n2<0){
					if (errno==EAGAIN)
						printf("%u: !! write NOT available\n", childs);
					else
						printf("%u: !! fail to send. error=%d\n", childs, errno);
				}
				else{
					lg-=n2;
					next+=n2;
					/*printf("%u: %d bytes sent\n", childs, n2);*/
				}
			}
		}
		if (next!=n){
			printf("%u: %d bytes ready to send, total %d bytes sent (%s)\n",
				childs, n, next, fn_list[i].fn);
		}
/*
		else{
			printf("%u: total %d bytes sent (%s)\n", childs, next, fn_list[i].fn);
		}
*/
	}

exit:
	if (outbuf[0]){
		write(fd, outbuf, strlen(outbuf));
		printf("%u: BYE--BY--TEARDOWN (maybe fail to send reply)\n", childs);
	}

	if (1){
		struct RTSP_interleaved_RTCP *c3;
		
		nanosleep(&delay, (struct timespec *)NULL);
		c3 = (struct RTSP_interleaved_RTCP *)buf2;
		c3->dollar='$';
		c3->channel_id=0;
		c3->payload_len=sizeof(struct RTCP_header);
		c3->rtcp_head.version=2;
		c3->rtcp_head.sc=0;
		c3->rtcp_head.p=0;
		c3->rtcp_head.pt=203; /* BYE */
		c3->rtcp_head.length=htonl(1);
		c3->rtcp_head.ssrc=ssrc_save;
		write(fd, buf2, sizeof(struct RTSP_interleaved_RTCP));
	}
	
	printf("%u: The End. (%s)\n", childs, date_str()); 
	free(buf2);
	free(fn_list);
	fflush(stdout);

	return 0;
}

/*------------------------------------------------------------------------
 * proxyd - process redirction
 *------------------------------------------------------------------------
 */
int proxyd(int fd)
{
	int n;
	time_t tt;

	time(&tt);
	
	printf("%u: NEW fork on (%s)\n", childs, date_str());
	
	while(1){
		/*  
		 * read	from CLIENT and	send to	SERVER	
		 */
		if ((n=read(fd,buf,1500))>0){
			buf[n]='\0';
/*
			printf("%u: <<<\n", childs);
			printf("%s\n", buf);
*/
			if (strstr(buf,	"HEARTBEAT")){
				printf("%u: <<< HEARTBEAT\n", childs);
				sprintf(outbuf, re3, get_cseq(buf), date_str());
			}		     
			else if	(strstr(buf, "DESCRIBE")){
				long long ttt;

				printf("%u: <<< DESCRIBE\n", childs);
				ttt=milli_time();
				sprintf(inbuf, sdp, ttt, ttt); /* use inbuf as tmp, only once */
				sprintf(outbuf, re0, get_cseq(buf), strlen(inbuf), inbuf);
			}
			else if	(strstr(buf, "SETUP")){
				printf("%u: <<< SETUP\n", childs);
				sprintf(outbuf, re1, get_cseq(buf), date_str());
			}
			else if	(strstr(buf, "PLAY")){ 
				/* 
				 * streaming request 
				 */
				printf("%u: <<< PLAY\n", childs);
				livestream(fd, buf);
				return 0;
			}
			else{ 
				/* 
				 * unknown request, return 400 
				 */
				return 0;
			}

			printf("%u: >>>\n", childs);
			/* printf("%s\n", outbuf);	*/

			write(fd, outbuf, strlen(outbuf));
			
			time(&tt);
		}
	
		if (time(NULL)-tt>10) { /* 10 seconds to timeout */
			printf("%u: ! close as TIMEOUT\n", childs);
			return 0;
		}
	}
}

/*------------------------------------------------------------------------
 * main	
 *------------------------------------------------------------------------
 */
int main(int argc, char	*argv[])
{
	struct sockaddr_in fsin; /* the	from address of	a client */
	char  pxy_service[10];	/* port	number of proxy	 */ 
	int msock, ssock;   /* master &	slave sockets */
	socklen_t alen;	  /* from-address length  */
	int cid, flags;
	
	printf("\nStreaming Server 1.0  ----  provide stream service for H.264 Kam\n");
	printf("written by jack139, F8 Network 2014 (%s)\n", date_str());
	
	switch (argc) {
	case  3:
		  strcpy(pxy_service, argv[1]);
		  strcpy(snap_path, argv[2]);
		  break;
	default:
		  printf("usage: streaming <service port> <snap_path>\n");
		  exit(0);
	}
	
	msock =	passiveTCP(pxy_service,	QLEN);
	if (msock==-1) exit(1);

	signal(SIGCHLD, SIG_IGN);
		
	printf("Listen on port %s ...\n\n", pxy_service);

	fflush(stdout);
		
	while (1) {
		ssock =	accept(msock, (struct sockaddr *)&fsin,	&alen);
		if (ssock < 0){
			printf("! accept failed\n");
			exit(1);
		}
		cid=fork();
		if (cid==-1){
			printf("! cannot create	child process");
			continue;
		}
	  
		childs++;
		
		if (cid>0){
			close(ssock);
		}
		else{
			flags = fcntl(ssock, F_GETFL, 0);
			fcntl(ssock, F_SETFL, flags | O_NONBLOCK);
			proxyd(ssock);
			close(ssock);
			exit(0);
		}
	}
}
