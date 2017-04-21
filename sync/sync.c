/* 
 * RTSP sync for 307
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
#include <dirent.h>
#include <errno.h>

#include "sync.h"
#include "http_lib.h"

#define	NAMLEN(dirent) strlen((dirent)->d_name)

#define ONE_SEC (1000)
#define MAX_SYNC (50)

pthread_mutex_t mutex_queue;

struct sync_kam{
	char local_camid[50];
	char last_put[20];
	char *sync_kamid;
};
struct sync_kam *sync_list;
int sync_num=0;

struct put_queue {
	struct put_queue *next;
	char *path;
	int index;
};
struct put_queue *queue_head=NULL, *queue_tail=NULL;
int queue_num=0;

char snap_path[200];

static char kam_server[30]={'\0'};
static char file_server[30]={'\0'};
static int init=1;

char *cache_buf;

char *http_req[2], *jpg_buf;
char k_kamip[20];
char k_camid[30];
char k_auth[30];
long k_delay=-1;
int k_motion_detect=0;
char g_tmp[200];

char *date_str(void)
{
	time_t now;
	struct tm *tm_now;
 
	time(&now);
	tm_now = localtime(&now);
	
	strftime(g_tmp,	100, "%a, %b %d %Y %H:%M:%S %Z", tm_now);

	return g_tmp;
}

/*
 *  config file format:
 *
 *  5337d71685cb3115b16c53ce S307UCC9KT73\n
 *  $$$\n
 */
int read_config(char *fn_name)
{
	FILE *pf;
	int num=0;
	
	printf("read config: %s\n", fn_name);
	
	pf = fopen(fn_name,"r");
	if(pf==NULL){
		printf("config file open fail! errno=%d\n", errno);
		exit(1);
	}
	while(fgets(sync_list[num].local_camid,50,pf)!=NULL){
		if (sync_list[num].local_camid[0]=='$') break;
		sync_list[num].local_camid[24]='\0';
		sync_list[num].sync_kamid=sync_list[num].local_camid+25;
		sync_list[num].sync_kamid[12]='\0';
		sync_list[num].last_put[0]='\0';
		printf("[%s] --> [%s]\n", sync_list[num].local_camid, sync_list[num].sync_kamid);
		num++;
		if (num==MAX_SYNC) break;
	}
	
	fclose(pf);

	return num;
}

int hand_shake(char *server_to_hand, char *kamid)
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

int fileserv(void)
{
  const char delims[] = "\n";
  int  ret,lg,ret2=-1;
  char typebuf[30];  
  char *filename=NULL;
  char *result = NULL;
  
  sprintf(http_req[P_MAIN], "http://%s/kam/fileserv", kam_server);
  ret=http_parse_url(http_req[P_MAIN],&filename,P_MAIN);
  if (ret<0) return ret;
  
  ret=http_get(filename,&lg,typebuf,P_MAIN);
  
#if 0
  printf("fileserv: res=%d,type='%s',lg=%d\n",ret,typebuf,lg);
#endif  

  if (ret==200){
    data_buf[P_MAIN][lg]='\0';    
    result = strtok( data_buf[P_MAIN], delims );
    while( result != NULL ) {
      sscanf(result, "ret=%d", &ret2);
      if (ret2<0) break;
      sscanf(result, "serv=%s", typebuf);
  
#if 0
      printf("fileserv: %s\n", result);
#endif    
  
      result = strtok( NULL, delims );
    }
    sprintf(file_server, "%s:20132", typebuf);
  }  
  
  return ret2;
}

int conf(void)
{
  const char delims[] = "\n";
  int  ret,lg,ret2=-1;
  char typebuf[30];  
  char *filename=NULL;
  char *result = NULL;  
  long long timee=0LL;
  
  get_ip();  /* get local ip address */
  
  sprintf(http_req[P_MAIN], "http://%s/kam/conf?lost=0", file_server);
  ret=http_parse_url(http_req[P_MAIN],&filename,P_MAIN);
  if (ret<0) return ret;
  
  ret=http_get(filename,&lg,typebuf,P_MAIN);
  
#ifdef DEBUG    
  printf("conf: res=%d,type='%s',lg=%d\n",ret,typebuf,lg);
/*  
  printf("conf: %s", data);
*/
#endif  

  if (ret==200){
    data_buf[P_MAIN][lg]='\0';
    result = strtok( data_buf[P_MAIN], delims );
    while( result != NULL ) {
      sscanf(result, "ret=%d", &ret2);
      if (ret2<0) break;
      sscanf(result, "delay=%ld", &k_delay);
      sscanf(result, "motion_detect=%d", &k_motion_detect);
      sscanf(result, "timee=%lld", &timee);
  
#ifdef DEBUG      
      printf("conf: %s\n", result );
#endif    
  
      result = strtok( NULL, delims );
    }
  }  
  
  return ret2;
}

/* get last filename */
int fc_last(char *path)
{
	DIR *dp,*dp2;
	struct dirent *ep, *ep2;
	char tmp[100], max_f[20], max_f2[20], max_d[10];

	strcpy(max_f, "0");
	strcpy(max_f2, "0");
	strcpy(max_d, "0");

	dp = opendir(path);
	if (dp != NULL){
		while ((ep = readdir(dp))){
			if (ep->d_name[0]=='.' && 
				(NAMLEN(ep)==1 ||(ep->d_name[1]=='.' &&	NAMLEN(ep)==2))) continue;
			if (strcmp(ep->d_name, "capture")==0) continue;
			if (strcmp(ep->d_name, max_d)<0) continue;
			strcpy(max_d, ep->d_name);
		}
		(void) closedir	(dp);
		
		sprintf(tmp, "%s/%s", path, max_d);
		dp2 = opendir (tmp);
		if (dp2	!= NULL){
			while ((ep2 = readdir(dp2))){
				if (ep2->d_name[0]=='.'	&& 
					(NAMLEN(ep2)==1	||
						(ep2->d_name[1]=='.' &&	NAMLEN(ep2)==2))) continue;
				if (strcmp(ep2->d_name, max_f2)<0) continue;
				if (strcmp(ep2->d_name, max_f)<0)
					strcpy(max_f2, ep2->d_name);
				else{
					strcpy(max_f2, max_f);
					strcpy(max_f, ep2->d_name);
				}
			}
			(void) closedir	(dp2);
			
			/* retuen whole path in g_tmp */
			sprintf(g_tmp, "%s/%s/%s", path, max_d, max_f2);
			
			//printf("fc_last: [%s] [%s]\n", max_f, max_f2);
		}
		else
			return -2;
	}
	else
		return -1;

	return 0;
}

int check_put_ready(int sync_i){  /* ret=1 - ready, 0 - not ready, -n - fail */
	char cam_path[100];
	int ret, i;
	char *p;

	/* get filename ready to send */		
	sprintf(cam_path, "%s/%s", snap_path, sync_list[sync_i].local_camid);
	ret=fc_last(cam_path);
	if (ret<0){
		printf("check_put_ready: %s fc_last() fail. ret=%d\n", sync_list[sync_i].sync_kamid, ret);
		return ret;
	}

	/* extract filename */
	for(i=strlen(g_tmp);i>0 && g_tmp[i]!='/';i--);
	p=g_tmp+i+1;

	if (strcmp(p, "0")==0){
		/* first file in dir */
		return 0;
	}
	
	/* whether is last sent? */
	if (strcmp(sync_list[sync_i].last_put, p)==0){
		//printf("check_put_ready: %s repeated, ignored.\n", sync_list[sync_i].sync_kamid);
		return 0;
	}
	
	/* at this moment, the file in lat_put has not put */
	strcpy(sync_list[sync_i].last_put, p);
	return 1;
}

int put(struct put_queue *node)
{
	int  ret, i, fd2, n;
	char *filename=NULL, *g, *p;
	//char cam_path[100];

	g=node->path;

	/* extract filename */
	for(i=strlen(g);i>0 && g[i]!='/';i--);
	p=g+i+1;

	if (strcmp(p, "0")==0){
		/* first file in dir */
		free(node->path);
		free(node);
		return 0;
	}
	
	/* g like "/usr/data3/snap_store/5337d71685cb3115b16c53ce/2326948/1396168854.9327" */
	fd2=open(g, 0);
	n=read(fd2, jpg_buf, 1024*JPG_BUF_SIZE-1);
	if (n==-1){
		printf("file read fail! errno=%d path=%s\n", errno, g);
		close(fd2);
		free(node->path);
		free(node);
		return n;
	}
	close(fd2);
	
	sprintf(jpg_buf+n, "%8d|%20lld|%9d#", k_motion_detect, (long long)(strtod(p, NULL)*1000000L), n); 
	
	printf("put: %s [%s] q=%d\n", sync_list[node->index].sync_kamid, jpg_buf+n, queue_num);

	free(node->path);
	free(node);

	/* send PUT */
	sprintf(http_req[P_MAIN], "http://%s/kam/put264", file_server);
	ret=http_parse_url(http_req[P_MAIN],&filename,P_MAIN);
	if (ret<0) return ret;

	ret=http_put(filename,jpg_buf,n+40,0,NULL); /* (jpg length + 40 chars) */

#ifdef DEBUG      
	printf("put: res=%d\n",ret);
#endif

	return (ret==200) ? 0 : ((ret<0)?ret:-ret);
}

void *check_thread(void)
{
	struct timespec	delay2;
	int j;
	
	queue_head=NULL;
	queue_tail=NULL;
	queue_num=0;

	delay2.tv_sec=0;
	delay2.tv_nsec=500*1000*1000; 
	
	while(1){
		for(j=0; j<sync_num; j++){
			if (check_put_ready(j)==1){
				struct put_queue *node;
				
				if (!(node=malloc(sizeof(struct put_queue)))){
					printf("check_thread: malloc fail! thread quit.");
					goto finnal;
				}
				node->path=strdup(g_tmp);
				node->index=j;
				node->next=NULL;

				pthread_mutex_lock(&mutex_queue);
				if (queue_num==0){
					queue_head=node;
					queue_tail=node;
					queue_num++;
				}
				else{
					queue_tail->next=node;
					queue_tail=node;
					queue_num++;
				}
				//printf("add_new: %s %s q=%d\n", sync_list[node->index].sync_kamid, node->path, queue_num);
				pthread_mutex_unlock(&mutex_queue);
			}
			if (nanosleep(&delay2, (struct timespec *)NULL)!=0)
				printf("check_thread: nanosleep() was interrupted!\n");
		}
	}

finnal:	
	return NULL;
}


int main(int argc, char **argv) 
{
	pthread_t id;
	int ret, i;
	struct timespec delay1, delay2;
	
	printf("\nRec 1.0  ----  provide synchronize service for H.264 Kam\n");
	printf("written by jack139, F8 Network 2014 (%s)\n", date_str());

	if (argc<3) {
		printf("Usage: rec <serv_ip> <cam_list_file> <snap_path>\n");
		return 0;
	}
  
	sprintf(kam_server, "%s:20133", argv[1]);
	strcpy(snap_path, argv[3]);
	printf("Server: %s   Config_file: %s snap_path: %s\n\n", kam_server, argv[2], snap_path);

	/* allocat buffers */
	if (!(cache_buf=malloc(CACHE_BUF_SIZE))) return 1;
	if (!(jpg_buf=malloc(1024*(JPG_BUF_SIZE+1)))) return 1; 
	if (!(sync_list=malloc(sizeof(struct sync_kam)*(MAX_SYNC+1)))) return 1; 
	for(i=0;i<2;i++){
		if (!(http_req[i]=malloc(HEADER_SIZE))) return 1;
		if (!(http_server[i]=malloc(MAXBUF))) return 1; 
		if (!(http_filename[i]=malloc(MAXBUF))) return 1; 
		if (!(data_buf[i]=malloc(1024*JPG_BUF_SIZE))) return 1; 
		if (!(http_header[i]=malloc(HEADER_SIZE))) return 1; 
		if (!(query_header[i]=malloc(HEADER_SIZE))) return 1; 
	}

	sync_num=read_config(argv[2]);

	delay1.tv_sec=1;
	delay1.tv_nsec=200*1000*1000;
	delay2.tv_sec=2;
	delay2.tv_nsec=500*1000*1000;

	k_delay=-1;
	
	pthread_mutex_init (&mutex_queue,NULL);

	ret=pthread_create(&id,NULL,(void *)check_thread,NULL);
	if(ret!=0){
		printf("Create pthread check_thread() error!\n");
		return 1;
	}

	while(sync_num){
re_hand:		
		fflush(stdout);
		
		//sprintf(kamid, "%s", sync_list[0].sync_kamid);
		
		/* first hand shake */
		do{
			for(i=0; i<1200; i++){
				printf("main: first hand_shake ... %s (%d)\n", kam_server, i);
				ret=hand_shake(kam_server, sync_list[0].sync_kamid);
				if (ret==0) break;
				sleep(3);
			}
			if (i==1200) goto exit;  /* reboot if hand shake fail for 1 hour */
		
			ret=fileserv();
		}while(ret);
	
		printf("file_server=%s\n", file_server);
	
		while(1){
			struct put_queue *node;
			
			fflush(stdout);
			
			/* check snap_thread is alive or dead ? */
			if (pthread_kill(id, 0)!=0){
				printf("main: snap_thread is dead :( ... will reboot.\n");
				goto exit;
			}

			//sprintf(kamid, "%s", sync_list[j].sync_kamid);

			pthread_mutex_lock(&mutex_queue);
			if (queue_num==0){ /* No need to put */
				pthread_mutex_unlock(&mutex_queue);
			}
			else{
				/* pop a node from queue */
				node=queue_head;
				queue_head=node->next;
				queue_num--;
				pthread_mutex_unlock(&mutex_queue);

				/* hand shake */
				/* printf("main: hand_shake ... %s (%d) - %s\n", file_server, i, sync_list[node->index].sync_kamid); */
				ret=hand_shake(file_server, sync_list[node->index].sync_kamid);
				if (ret!=0){
					printf("main: handshake() fail! ret=%d (%s)\n", ret, sync_list[node->index].sync_kamid);
					free(node->path);
					free(node);
					goto re_hand; /* file server handshake fail */
				}

				/* get conf */
				if ((ret=conf())<0){
					printf("main: conf() fail! ret=%d (%s)\n", ret, sync_list[node->index].sync_kamid);
					free(node->path);
					free(node);
					continue; /* next sync */
				}

				/* show conf info */
				/* printf("conf: (%s) %ld %d\n", date_str(), k_delay, k_motion_detect); */
		
				/* put snapshot */
				if (k_delay!=-1){
					if ((ret=put(node))<0){
						printf("main: put() fail! ret=%d (%s)\n", ret, sync_list[node->index].sync_kamid);
					}
				} 
			}

			if (queue_num<3){
				if (nanosleep(&delay2, (struct timespec *)NULL)!=0)
					printf("main: nanosleep() was interrupted!\n");
			}
			else{
				if (nanosleep(&delay1, (struct timespec *)NULL)!=0)
					printf("main: nanosleep() was interrupted!\n");
			}
		}
	}

exit:
	pthread_join(id,NULL);

	if (queue_num>0){ /* free allocated memory */
		struct put_queue *t;
		
		pthread_mutex_lock(&mutex_queue);
		t=queue_head;
		while(t){
			t=queue_head->next;
			free(queue_head->path);
			free(queue_head);
			queue_head=t;
		}
		queue_num=0;
		pthread_mutex_unlock(&mutex_queue);
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
	free(sync_list);

	puts("sync exit.");
	
	return 0;
}
