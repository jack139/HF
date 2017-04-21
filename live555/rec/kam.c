/* 
 * RTSP recorder
 * 
 * F8 Network (c) 2014, jack139
 *
 * Version: 1.0
 *
 */

#include <sys/types.h>
#include <sys/uio.h>
#include <sys/timeb.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h> 
#include <pthread.h>
#include <signal.h>
#include <time.h>

#include "kam.h"
#include "http_lib.h"

#define ONE_SEC (1000)

char kam_server[30]={'\0'};
int init=1;

char *cache_buf=NULL;

pthread_mutex_t mutex_first;

char kamid[15];

char *http_req[2], *jpg_buf;
char k_kamip[20];
char k_camid[30];
char k_auth[30];
long k_delay=-1;
int k_motion_detect=0;
char k_path[100];
char g_tmp[200];

long long milli_time(void)
{
  struct timeval t2;

  if (gettimeofday(&t2, (struct timezone *)NULL) == 0)
    return (long long)t2.tv_sec * 1000000LL + (long long)t2.tv_usec;
  else
    return 0;
}

char *date_str(void)
{
	time_t now;
	struct tm *tm_now;
 
	time(&now);
	tm_now = localtime(&now);
	
	strftime(g_tmp,	100, "%a, %b %d %Y %H:%M:%S %Z", tm_now);

	return g_tmp;
}

int hand_shake(char *server_to_hand)
{
  int  ret,lg;
  char typebuf[30];
  char *filename=NULL;
  int  login=0;
  
  sprintf(http_req[P_MAIN], "http://%s/kam/handshake?kamid=%s&init=%d", server_to_hand, kamid, init);
  ret=http_parse_url(http_req[P_MAIN],&filename,P_MAIN);
  if (ret<0) return ret;
  
  ret=http_get(filename,&lg,typebuf,P_MAIN);
  
#ifdef DEBUG  
  printf("hand_shake: res=%d,type='%s',lg=%d\n",ret,typebuf,lg);
#endif
  
  if (ret==200){
    data_buf[P_MAIN][lg]='\0';
    sscanf(data_buf[P_MAIN], "login=%d", &login);
    init=0;
  }
  else
    login=0;
  
  return (login==1) ? 0 : -1;
}

int conf(void)
{
  const char delims[] = "\n";
  int  ret,lg,ret2=-1;
  char typebuf[30];  
  char *filename=NULL;
  char *result = NULL; 
  
  get_ip();  /* get local ip address */
  
  sprintf(http_req[P_MAIN], "http://%s/kam/conf", kam_server);
  ret=http_parse_url(http_req[P_MAIN],&filename,P_MAIN);
  if (ret<0) return ret;
  
  ret=http_get(filename,&lg,typebuf,P_MAIN);
  
#ifdef DEBUG    
  printf("conf: res=%d,type='%s',lg=%d\n",ret,typebuf,lg);
/*  
  printf( "conf: %s", data);
*/
#endif  

  if (ret==200){
    data_buf[P_MAIN][lg]='\0';
    result = strtok( data_buf[P_MAIN], delims );
    while( result != NULL ) {
      sscanf(result, "ret=%d", &ret2);
      if (ret2<0) break;
      sscanf(result, "delay=%ld", &k_delay);
      sscanf(result, "motion=%d", &k_motion_detect);
      sscanf(result, "camid=%s", k_camid);
      sscanf(result, "auth=%s", k_auth);
      sscanf(result, "kamip=%s", k_kamip);
      sscanf(result, "path=%s", k_path);
  
#ifdef DEBUG      
      printf("conf: %s\n", result );
#endif    
  
      result = strtok( NULL, delims );
    }
  }  
  
  return ret2;
}

int malloc_all(void)
{
	int i;
	
	/* allocat buffers */
	if (!(cache_buf=malloc(CACHE_BUF_SIZE))) return -1;
	if (!(jpg_buf=malloc(1024*(JPG_BUF_SIZE+1)))) return -1;
	for(i=0;i<2;i++){
		if (!(http_req[i]=malloc(HEADER_SIZE))) return -1;
		if (!(http_server[i]=malloc(MAXBUF))) return -1; 
		if (!(http_filename[i]=malloc(MAXBUF))) return -1; 
		if (!(data_buf[i]=malloc(1024*JPG_BUF_SIZE))) return -1; 
		if (!(http_header[i]=malloc(HEADER_SIZE))) return -1; 
		if (!(query_header[i]=malloc(HEADER_SIZE))) return -1; 
	}
	
	return 0;
}

void free_all(void)
{
	int i;
	
	for(i=0;i<2;i++){
		free(http_server[i]);
		free(http_filename[i]);
		free(data_buf[i]);
		free(http_header[i]);
		free(query_header[i]);
		free(http_req[i]);
	}
	free(cache_buf);
	free(jpg_buf);
}
