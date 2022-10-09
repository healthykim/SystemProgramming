#include <stdio.h>
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400
#define CACHE_LINES 10

struct cache_line {
    char cache_payload[MAX_OBJECT_SIZE];
    char cache_url[MAXLINE];
    int cnt_lru;
    int valid;

};

struct cache_struct {
    struct cache_line cache_lines[CACHE_LINES];  /*ten cache blocks*/
};

struct cache_struct cache;

void cache_init();
int cache_hit(char *url);
int cache_evict();
void cache_write(char *uri,char *buf);
void lru_update(int index);

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
static const char *host_hdr = "Host: %s\r\n";
static const char *requestLine = "GET %s HTTP/1.0\r\n";
static const char *connection_hdr = "Connection: close\r\n";
static const char *proxy_hdr = "Proxy-Connection: close\r\n";
static const char *endline = "\r\n";

void doit(int fd);
void *thread(int * fdp);
void parse_uri(char *uri,char *prefix,char *suffix,char *port);
void read_requesthdrs(rio_t *rp);
void getHeader(char *header, char *hostname, char *path, int port, rio_t *client_rio);
int connect_origin(char *hostname, int port);

int main(int argc, char **argv)
{
    int listenfd, *connfdp;
    char hostname[MAXLINE], port[MAXLINE];
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    pthread_t tid;
    
    /* Check command line args */
    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }
    cache_init();


    /*1. Socket & Bind & Listen*/
    listenfd = Open_listenfd(argv[1]);
    while(1) {
        clientlen = sizeof(clientaddr);
        connfdp = Malloc(sizeof(int));
        *connfdp = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        Getnameinfo((SA *) &clientaddr, clientlen, hostname, MAXLINE, 
            port, MAXLINE, 0);
        printf("Accepted connection from (%s, %s)\n", hostname, port);
        Pthread_create(&tid, NULL, thread, connfdp);  
    }
    return 0;
}

void *thread(int *fdp){
    int connfd = *fdp;
    Pthread_detach(pthread_self());
    doit(connfd);
    Close(connfd);
    return;
}

void doit(int fd){
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE], prefix[MAXLINE], suffix[MAXLINE], port[MAXLINE];
    char buf_origin[MAXLINE];

    rio_t rio_client, rio_origin;

    /* 1. Get request line */
    Rio_readinitb(&rio_client, fd);
    if (!Rio_readlineb(&rio_client, buf, MAXLINE))
        return;
    sscanf(buf, "%s %s %s", method, uri, version);    

    if (strcasecmp(method, "GET")!=0) {     
        printf("not implemented\n");            
        return;
    }

    int cache_index = cache_hit(uri);
    if(cache_index >= 0){
        Rio_writen(fd, cache.cache_lines[cache_index].cache_payload, strlen(cache.cache_lines[cache_index].cache_payload));
        lru_update(cache_index);
        return;
    }

    parse_uri(uri, prefix, suffix, &port);   

    /* 2. Forward request to origin server */

    /*GET path HTTP-version*/
    sprintf(buf_origin, requestLine, suffix);
    
    int originfd = Open_clientfd(prefix, port);
    Rio_writen(originfd, buf_origin, strlen(buf_origin));

    /*Host: hostname*/
    sprintf(buf_origin, host_hdr, prefix);
    Rio_writen(originfd, buf_origin, strlen(buf_origin));

    /*User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n*/
    Rio_writen(originfd, user_agent_hdr, strlen(user_agent_hdr));

    /*Connection: close\r\n*/
    Rio_writen(originfd, connection_hdr, strlen(connection_hdr)); 
    
    /*Proxy-Connection: close\r\n*/
    Rio_writen(originfd, proxy_hdr, strlen(proxy_hdr));
    
    Rio_writen(originfd, endline, strlen(endline));

    Rio_readinitb(&rio_origin, originfd);

    size_t total=0;
    size_t n =0;
    char buf_cache[MAX_OBJECT_SIZE];
    while((n = (Rio_readlineb(&rio_origin, buf, MAXLINE)))!=0 ){
        Rio_writen(fd, buf, n);
        total += n;
        if(total<MAX_OBJECT_SIZE)
            strcat(buf_cache, buf);
    }
    if(total<MAX_OBJECT_SIZE){
        cache_write(uri, buf_cache);
    }
    Close(originfd);
}

void read_requesthdrs(rio_t *rp) 
{
    char buf[MAXLINE];

    Rio_readlineb(rp, buf, MAXLINE);
    while(strcmp(buf, "\r\n")) {
	Rio_readlineb(rp, buf, MAXLINE);
    }
    return;
}


/*parse the uri to get hostname,file path ,port*/
void parse_uri(char *uri,char *prefix,char *suffix,char *port)
{
    char* tmp1 = strstr(uri, "//");
    if(tmp1 == NULL){
        tmp1 = uri;
    }
    else{
        tmp1 += 2;
    }
    char* tmp2 = strstr(tmp1, ":");
    if(tmp2 == NULL){
        strncpy(port, "80", strlen("80"));

        tmp2 = strstr(tmp1, "/");
        if(tmp2 == NULL){
            strncpy(prefix, tmp1, strlen(tmp1));
        }
        else{
            strncpy(prefix, tmp1, tmp2-tmp1);
            strncpy(suffix, tmp2, strlen(tmp2));
        }
    }
    else{
        strncpy(prefix, tmp1, tmp2-tmp1);
        char* tmp3 = strstr(tmp1, "/");
        strncpy(port, tmp2+1, tmp3-tmp2-1);
        strncpy(suffix, tmp3, strlen(tmp3));
    }
    return;
}

/*cache*/
void cache_init()
{
    for(int i=0; i<CACHE_LINES; i++){
        cache.cache_lines[i].cnt_lru = 0;
        cache.cache_lines[i].valid = 0;
    }
    return;
}

int cache_hit(char *url)
{
    for(int i=0; i<CACHE_LINES; i++){
        if(cache.cache_lines[i].valid && strcmp(cache.cache_lines[i].cache_url, url)==0){
            return i;
        }
    }
    return -1;
}

int cache_evict(){
    int min = __INT32_MAX__;
    int index = 0;
    for(int i=0; i<CACHE_LINES; i++){
        if(cache.cache_lines[i].valid == 0){
            return i;
        }
        if(cache.cache_lines[i].cnt_lru < min){
            min = cache.cache_lines[i].cnt_lru;
            index = i;
        }
    }
    return index;
}

void cache_write(char *uri,char *buf){

    int i = cache_evict();

    strcpy(cache.cache_lines[i].cache_url,uri);
    strcpy(cache.cache_lines[i].cache_payload,buf);
    cache.cache_lines[i].valid = 1;
    cache.cache_lines[i].cnt_lru = __INT32_MAX__;
    lru_update(i);

}

void lru_update(int index){
    for(int i=0; i<CACHE_LINES; i++)    {
        if(i==index)
            continue;
        if(cache.cache_lines[i].valid && i!=index){
            cache.cache_lines[i].cnt_lru--;
        }
    }
}
