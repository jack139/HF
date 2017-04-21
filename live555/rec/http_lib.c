/*
 *  Http put/get mini lib
 *  written by L. Demailly
 *  Modified by Jack139
 *
 * Description : Use http protocol, connects to server to echange data
 *
 */

#undef VERBOSE

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#ifdef LINUX
#include <linux/sockios.h>
#include <linux/ethtool.h>
#endif
#include <netdb.h>
#include <ctype.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>

#include "http_lib.h"
#include "kam.h"

#define SERVER_DEFAULT "adonis"
#define DATA_BUF_SIZE (JPG_BUF_SIZE*1024)

#define INT_TO_ADDR(_addr) \
(_addr & 0xFF), \
(_addr >> 8 & 0xFF), \
(_addr >> 16 & 0xFF), \
(_addr >> 24 & 0xFF)

static int http_read_line (int fd,char *buffer, int max) ;
static int http_read_buffer (int fd,char *buffer, int max) ;

char *http_server[2], *http_filename[2], *http_header[2], *query_header[2], *data_buf[2];
int  http_port[2]={80,80};
static char *http_user_agent="F8-307/1.3 (an Kam client(HiK) from Kam@Cloud)";

static char webpy_session[100]="";
static int local_ip;

/*
 * read a line from file descriptor
 * returns the number of bytes read. negative if a read error occured
 * before the end of line or the max.
 * cariage returns (CR) are ignored.
 */
static int http_read_line (int fd, char *buffer, int max) 
{ /* not efficient on long lines (multiple unbuffered 1 char reads) */
  int n=0;
  
  while (n<max) {
    if (read(fd,buffer,1)!=1) {
      n= -n;
      break;
    }
    n++;
    if (*buffer=='\015') continue; /* ignore CR */
    if (*buffer=='\012') break;    /* LF is the separator */
    buffer++;
  }
  *buffer=0;
  
  return n;
}


/*
 * read data from file descriptor
 * retries reading until the number of bytes requested is read.
 * returns the number of bytes read. negative if a read error (EOF) occured
 * before the requested length.
 */
static int http_read_buffer (int fd, char *buffer, int length) 
{
  int n,r;
  
  for (n=0; n<length; n+=r) {
    r=read(fd,buffer,length-n);
    if (r==0) break;
    if (r<0) return -n;
    buffer+=r;
  }
  return n;
}


/*
 * socket connect in NONBLOCK mode
 */
int conn_nonb(int sockfd, const struct sockaddr_in *saptr, socklen_t salen, int nsec, int keep_nonblock)
{  
    int flags, n, error, code;  
    socklen_t len;  
    fd_set wset;  
    struct timeval tval;  
  
    flags = fcntl(sockfd, F_GETFL, 0);  
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);  
  
    error = 0;  
    if ((n = connect(sockfd, (struct sockaddr *)saptr, salen)) == 0) {  
        goto done;  
    } else if (n < 0 && errno != EINPROGRESS){  
        return (-1);  
    }  
  
    /* Do whatever we want while the connect is taking place */  
  
    FD_ZERO(&wset);  
    FD_SET(sockfd, &wset);  
    tval.tv_sec = nsec;  
    tval.tv_usec = 0;  
  
    if ((n = select(sockfd+1, NULL, &wset,   
                    NULL, nsec ? &tval : NULL)) == 0) {  
        close(sockfd);  /* timeout */  
        errno = ETIMEDOUT;  
        return (-1);  
    }  
  
    if (FD_ISSET(sockfd, &wset)) {  
        len = sizeof(error);  
        code = getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &len);  
        if (code < 0 || error) {  
            close(sockfd);  
            if (error)   
                errno = error;  
            return (-1);  
        }  
    } else {  
        printf("select error: sockfd not set");  
        exit(0);  
    }  
  
done:  
    if (!keep_nonblock) fcntl(sockfd, F_SETFL, flags);  /* restore file status flags */  
    return (0);  
}

typedef enum 
{
  CLOSE,  /* Close the socket after the query (for put) */
  KEEP_OPEN /* Keep it open */
} querymode;

/*
 * Pseudo general http query
 *
 * send a command and additional headers to the http server.
 *
 * Limitations: the url is truncated to first 256 chars and
 * the server name to 128 in case of proxy request.
 */
static http_retcode http_query(char *command, char *url,
			       char *additional_header, querymode mode, 
			       char* data, int length, int *pfd, int threadnum)
{
  int     s;
  struct  hostent *hp;
  struct  sockaddr_in     server;
  int  hlg;
  http_retcode ret;
  int  port = http_port[threadnum];
  struct linger so_linger;
  
  if (pfd) *pfd=-1;
  
  /* get host info by name :*/
  if ((hp = gethostbyname( http_server[threadnum] ? http_server[threadnum]
				                 : SERVER_DEFAULT )
                         )) {
    memset((char *) &server,0, sizeof(server));
    memmove((char *) &server.sin_addr, hp->h_addr, hp->h_length);
    server.sin_family = hp->h_addrtype;
    server.sin_port = (unsigned short) htons( port );
  } else
    return ERRHOST;
  
  /* create socket */
  if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0) return ERRSOCK;
  setsockopt(s, SOL_SOCKET, SO_KEEPALIVE, 0, 0);
  so_linger.l_onoff = 1;
  so_linger.l_linger = 0;
  setsockopt(s, SOL_SOCKET, SO_LINGER, &so_linger, sizeof(so_linger));
  
  /* connect to server */
  /*if (connect(s, &server, sizeof(server)) < 0) ret=ERRCONN; */
  if (conn_nonb(s, &server, sizeof(server), 5, 0) < 0) ret=ERRCONN;
  else {
    if (pfd) *pfd=s;
    
    /* create header */
    if (webpy_session[0])
      sprintf(query_header[threadnum],
          "%s /%.256s HTTP/1.0\015\012"
          "Host: %.128s\015\012"
          "User-Agent: %s\015\012"
          "%s"
          "Cookie: web_session=%s\015\012\015\012",
  	      command, url,
  	      http_server[threadnum],   /* Host: xx.xx.xx.xx */
  	      http_user_agent,
  	      additional_header,
  	      webpy_session
  	  );
    else    
      sprintf(query_header[threadnum],
          "%s /%.256s HTTP/1.0\015\012"
          "Host: %.128s\015\012"
          "User-Agent: %s\015\012"
          "%s\015\012",
  	      command, url,
  	      http_server[threadnum],   /* Host: xx.xx.xx.xx */
  	      http_user_agent,
  	      additional_header
  	  );
    hlg=strlen(query_header[threadnum]);

#ifdef VERBOSE
    printf("http_lib.c: %s\n", query_header[threadnum]);
#endif	
    
    /* send header */
    if (write(s,query_header[threadnum],hlg)!=hlg) ret= ERRWRHD;
    
    /* send data */
    else if (length && data && (write(s,data,length)!=length)) ret= ERRWRDT;
    else {
      
      /* read result & check */
      ret=http_read_line(s,query_header[threadnum],MAXBUF-1);

#ifdef VERBOSE
      printf("http_lib.c: %s\n", query_header[threadnum]);
#endif	

      if (ret<=0) ret=ERRRDHD;
      else if (sscanf(query_header[threadnum],"HTTP/1.%*d %03d",(int*)&ret)!=1) ret=ERRPAHD;
      else if (mode==KEEP_OPEN) return ret;
    }
  }
  
  /* close socket */
  close(s);
  return ret;
}


/*
 * Put data on the server
 *
 * This function sends data to the http data server.
 * The data will be stored under the ressource name filename.
 * returns a negative error code or a positive code from the server
 *
 * limitations: filename is truncated to first 256 characters 
 *              and type to 64.
 */
http_retcode http_put(char *filename, char *data, int length, int overwrite, char *type) 
{
 /* only main thread use PUT */
  if (type) 
    sprintf(http_header[P_MAIN],
      "Content-length: %d\015\012"
      "Content-type: %.64s\015\012"
      "%s",
	    length,
	    type  ,
	    overwrite ? "Control: overwrite=1\015\012" : ""
	    );
  else
    sprintf(http_header[P_MAIN],
      "Content-length: %d\015\012"
      "%s",
      length,
	    overwrite ? "Control: overwrite=1\015\012" : ""
	    );
  return http_query("PUT",filename,http_header[P_MAIN],CLOSE, data, length, NULL, P_MAIN);
}


/*
 * Get data from the server
 *
 * This function gets data from the http data server.
 * The data is read from the ressource named filename.
 * Address of new new allocated memory block is filled in data_buf
 * whose length is returned via plength.
 * 
 * returns a negative error code or a positive code from the server
 * 
 *
 * limitations: filename is truncated to first 256 characters
 */
http_retcode http_get(char *filename, int *plength, char *typebuf, int threadnum)
{
  http_retcode ret;
  char *pc;
  int  fd;
  int  n,length=-1;

  if (plength) *plength=0;
  if (typebuf) *typebuf='\0';

  
  if (threadnum==P_MAIN){  /* only main thread send additional head parameters */
    sprintf(http_header[threadnum], 
      "X-local-addr: %d.%d.%d.%d\015\012",
      INT_TO_ADDR(local_ip));
  }
  else
    http_header[threadnum][0]='\0';
    
  ret=http_query("GET",filename,http_header[threadnum],KEEP_OPEN, NULL, 0, &fd, threadnum);
  if (ret==200) {
    while (1) {
      n=http_read_line(fd,http_header[threadnum],MAXBUF-1);
      
#ifdef VERBOSE
      printf("http_lib.c: %s\n", http_header[threadnum]);  
#endif
	
      if (n<=0) {
      	close(fd);
      	return ERRRDHD;
      }
      /* empty line ? (=> end of header) */
      if ( n>0 && (*(http_header[threadnum]))=='\0') break;
      /* try to parse some keywords : */
      /* convert to lower case 'till a : is found or end of string */
      for (pc=http_header[threadnum]; (*pc!=':' && *pc) ; pc++) *pc=tolower(*pc);
      sscanf(http_header[threadnum],"content-length: %d",&length);
      sscanf(http_header[threadnum],"set-cookie: web_session=%[^;];",webpy_session);
      if (typebuf) sscanf(http_header[threadnum],"content-type: %s",typebuf);
    }
    /* Nginx's trunked reply do not have Content-length */
    if (length<=0 || length>DATA_BUF_SIZE)
      length=DATA_BUF_SIZE;

    n=http_read_buffer(fd,data_buf[threadnum],length);
    if (plength) *plength=n;
    close(fd);

    /* if (n!=length) ret=ERRRDDT; */
  } else if (ret>=0) close(fd);
  return ret;
}


/* parses an url : setting the http_server and http_port global variables
 * and returning the filename to pass to http_get/put/...
 * returns a negative error code or 0 if sucessfully parsed.
 */
http_retcode http_parse_url(char *url, char **pfilename, int threadnum)
{
  char *pc,c;
  
  http_port[threadnum]=80;
  
  if (strncasecmp("http://",url,7)) {
    
#ifdef VERBOSE
    printf("http_lib.c: invalid url (must start with 'http://')\n");
#endif

    return ERRURLH;
  }
  url+=7;
  for (pc=url,c=*pc; (c && c!=':' && c!='/');) c=*pc++;
  *(pc-1)=0;
  if (c==':') {
    if (sscanf(pc,"%d",&(http_port[threadnum]))!=1) {

#ifdef VERBOSE
      printf("http_lib.c: invalid port in url\n");
#endif

      return ERRURLP;
    }
    for (pc++; (*pc && *pc!='/') ; pc++) ;
    if (*pc) pc++;
  }

  strcpy(http_server[threadnum], url);
  strcpy(http_filename[threadnum], (c ? pc : ""));
  *pfilename = http_filename[threadnum];

#ifdef VERBOSE
  printf("http_lib.c: host=(%s), port=%d, filename=(%s)\n",
	    http_server[threadnum],http_port[threadnum],*pfilename);
#endif

  return OK0;
}


/* if_name like "ath0", "eth0". Notice: call this function need root privilege.
  return value:
  -1 -- error , details can check errno
   1 -- interface link up
   0 -- interface link down.
*/
int get_netlink_status(const char *if_name)
{
#ifdef LINUX

    int skfd;
    struct ifreq ifr;
    struct ethtool_value edata;
    edata.cmd = ETHTOOL_GLINK;
    edata.data = 0;
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, if_name, sizeof(ifr.ifr_name) - 1);
    ifr.ifr_data = (char *) &edata;
    if (( skfd = socket( AF_INET, SOCK_DGRAM, 0 )) == 0){
        /* printf("errno=%d %s\n",errno,strerror(errno));*/
        return -1;
    }
    if(ioctl( skfd, SIOCETHTOOL, &ifr ) == -1)
    {
        close(skfd);
        /* printf("errno=%d %s\n",errno,strerror(errno)); */
        return -1;
    }
    close(skfd);
    return edata.data;
#else
    return 1;
#endif
}

int get_ip(void)
{
    struct ifconf ifc;
    struct ifreq ifr[10];
    int sd, ifc_num, i;

    /* Create a socket so we can use ioctl on the file 
     * descriptor to retrieve the interface info. 
     */

    sd = socket(PF_INET, SOCK_DGRAM, 0);
    if (sd > 0)
    {
        ifc.ifc_len = sizeof(ifr);
        ifc.ifc_ifcu.ifcu_buf = (caddr_t)ifr;

        if (ioctl(sd, SIOCGIFCONF, &ifc) == 0)
        {
            ifc_num = ifc.ifc_len / sizeof(struct ifreq);

            for (i = 0; i < ifc_num; ++i)
            {
                if (ifr[i].ifr_addr.sa_family != AF_INET) continue;
                if (ifr[i].ifr_name[0]=='l') continue;  /* 'lo' exclusive */

                /* ra0  - wireless card, 
                          if up, get_netlink_status() will return -1.
                   eth0 - 100M card
                          always up, connected get_netlink_status() will return 1.
                                     disconnected get_netlink_status() will return 0.
                */
                
                /* Retrieve the IP address */
                if (ioctl(sd, SIOCGIFADDR, &ifr[i])==0 && get_netlink_status(ifr[i].ifr_name)!=0)
                {
                    /* return first ip address */
                    local_ip = ((struct sockaddr_in *)(&ifr[i].ifr_addr))->sin_addr.s_addr;
                    /* printf("%d.%d.%d.%d\n", INT_TO_ADDR(local_ip)); */
                    close(sd);
                    return 0;
                }
            }                      
        }

        close(sd);
    }

    return -1;
}

