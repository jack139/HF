/* 
 * RTSP recorder
 * 
 * F8 Network (c) 2014, jack139
 *
 * Version: 1.0
 *
 */
#include <pthread.h>

#define MAXBUF 512
#define HEADER_SIZE 2048
#define JPG_BUF_SIZE 1024

#define CACHE_SIZE 20
#define CACHE_BUF_SIZE (1024*1024)

#define P_MAIN 0
#define P_SNAP 1

extern char *cache_buf;

extern char *http_req[2];
extern char k_kamip[20];
extern char k_camid[30];
extern char k_auth[30];
extern long k_delay;
extern char k_path[100];

extern pthread_mutex_t mutex_first;

long long milli_time(void);