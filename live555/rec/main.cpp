/* 
 * RTSP recorder
 * 
 * F8 Network (c) 2014, jack139
 *
 * Version: 1.0
 *
 */

#include <stdio.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>

extern "C" {
#include "kam.h"
#include "http_lib.h"
}

#define ONE_SEC (1000)

void *snap_thread(void *arg);

int main(int argc, char **argv) 
{
  pthread_t id;
  int ret, i, reboot;
  long long start_t, now_t;

  printf("\nRec 2.0  ----  provide recorder service for H.264 Kam (library from live555.com)\n");
  printf("written by jack139, F8 Network 2014 (%s)\n\n", date_str());

  if (argc<3) {
    printf("Usage: rec <serv_ip> <cam_reg_no>\n");
    return 0;
  }
  
  sprintf(kam_server, "%s:80", argv[1]);
  sprintf(kamid, "%s", argv[2]);
  printf("Server: %s   Kam ID: %s\n\n", kam_server, kamid);

  /* allocat buffers */
  if (malloc_all()<0) return 1;

first:
  k_delay=-1;
  
  pthread_mutex_init (&mutex_first,NULL);

  ret=pthread_create(&id,NULL, snap_thread,NULL);
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
        printf("conf: (%s) ", date_str());
	/* show conf info */
        printf(" %s %s %ld %d\n", 
                  k_camid, k_kamip, k_delay, k_motion_detect);
/*
        printf(" %s %s %s %ld %d %s\n", 
                  k_camid, k_kamip, k_auth, k_delay, k_motion_detect, k_path);
*/
      }

      sleep(1);
      fflush(stdout);
    }
  
  }  

exit:
  pthread_join(id,NULL);
  
  if (reboot){
  	printf("main: RESTART main loop ...\n");
  	goto first;
  }

  free_all();  
  
  return 0;
}

