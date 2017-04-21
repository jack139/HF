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
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h> 
#include <pthread.h>
#include <signal.h>
#include <time.h>

#include "kam.h"
#include "http_lib.h"
#include "vaap264.h"

#define ONE_SEC (1000)

/*
struct thread_cfg {
        int manager_stack_size;
        int each_thread_stack_size;
        int main_thread;        
};

struct thread_cfg thcfg={
        manager_stack_size:8*1024,
        each_thread_stack_size:2*1024,
        main_thread:2*1024,
};
*/

static char kam_server[30]={'\0'};
static int init=1;

char *cache_buf;

pthread_mutex_t mutex_first;

static char kamid[15];

char *http_req[2], *jpg_buf;
char k_kamip[20];
char k_camid[30];
char k_auth[30];
long k_delay=-1;
int k_motion_detect=0;
char k_path[100];
char g_tmp[200];

long long milli_time(void){
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

int main(int argc, char **argv) 
{
  pthread_t id;
  int ret, i, reboot;
  long long start_t, now_t;

  printf("\nRec 1.0  ----  provide recorder service for H.264 Kam\n");
  printf("written by jack139, F8 Network 2014 (%s)\n", date_str());

  if (argc<3) {
    printf("Usage: rec <serv_ip> <cam_reg_no>\n");
    return 0;
  }
  
  sprintf(kam_server, "%s:80", argv[1]);
  sprintf(kamid, "%s", argv[2]);
  printf("Server: %s   Kam ID: %s\n\n", kam_server, kamid);

  /* allocat buffers */
  if (!(cache_buf=malloc(CACHE_BUF_SIZE))) return 1;
  if (!(jpg_buf=malloc(1024*(JPG_BUF_SIZE+1)))) return 1; 
  for(i=0;i<2;i++){
    if (!(http_req[i]=malloc(HEADER_SIZE))) return 1;
    if (!(http_server[i]=malloc(MAXBUF))) return 1; 
    if (!(http_filename[i]=malloc(MAXBUF))) return 1; 
    if (!(data_buf[i]=malloc(1024*JPG_BUF_SIZE))) return 1; 
    if (!(http_header[i]=malloc(HEADER_SIZE))) return 1; 
    if (!(query_header[i]=malloc(HEADER_SIZE))) return 1; 
  }

first:
  k_delay=-1;
  
  pthread_mutex_init (&mutex_first,NULL);

  ret=pthread_create(&id,NULL,(void *)snap_thread,NULL);
  if(ret!=0){
    printf("Create pthread snap_thread() error!\n");
    return 1;
  }
  
  while(1){  
    /* hand shake */
    for(i=0; i<100; i++){
      printf("main: hand_shake ... %s (%d)\n", kam_server, i);
      ret=hand_shake(kam_server);
      if (ret==0) break;
      sleep(3);
    }
    if (i==100){
    	printf("main: file server handshake fail\n");
    	continue;
    }
   
    start_t=0LL;

    while(1){
      now_t=milli_time();
      if (now_t-start_t > ONE_SEC*60*1000){ /* get conf every minute */
        start_t=now_t;

        /* check snap_thread is alive or dead ? */
        if (pthread_kill(id, 0)!=0){
          printf("main: snap_thread is dead :( ... will reboot.\n");
          reboot=1;
          goto exit;
        }
        
        /* get conf */
        if ((ret=conf())<0){
          printf("main: conf() fail! ret=%d\n", ret);
          break; /* re-handshake */
        }

        /* show cache info */
        printf("conf: (%s) %u", date_str(), max_size);
	/* show conf info */
        printf(" %s %s %ld %d\n", 
                  k_camid, k_kamip, k_delay, k_motion_detect);
/*
        printf(" %s %s %s %ld %d %s\n", 
                  k_camid, k_kamip, k_auth, k_delay, k_motion_detect, k_path);
*/
      }

      sleep(1);
    }
  
  }  

exit:
  pthread_join(id,NULL);
  
  if (reboot){
  	printf("main: RESTART main loop ...\n");
  	goto first;
  }

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
  
  return 0;
}

