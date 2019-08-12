#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h> 
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <pthread.h>
#include <math.h>
#include "md5.h"

#define numservers 10   //预计10个服务器 
#define numnode 160*numservers   //建立节点 
#define LPORT 1234 
#define num 25 
//flag为1时为业务服务器，为2时为客户端
 
typedef struct
{
	int sock;
    char addr[22];                   //服务器ip地址
    int port;                    //服务器端口  
} nodeinfo; 
nodeinfo slist[numservers];

int ser_num=0;      //连接服务器数量 

// 以下数据结构就是continuum环上的结点，环上的每个点其实代表了一个ip地址，该结构把点和ip地址，端口一一对应起来。
// 环上的结点
typedef struct
{
    unsigned int point;          //在环上的点的哈希值 
    char addr[22];                       // 对应的ip地址
	int port;                //对应的端口 
	int sock;
} mcs; 
mcs continuum[numnode];      //预计创建节点个数 
int con_num=0;                      //实际建立的节点个数


pthread_t tid[num];
int t_num=0;

char ipbuf[22];

int quicksort(int begin, int end);
int create_continuum(void);
unsigned int get_hashi( char* inString );
int  find( char* key);
void swap(mcs *a,mcs  *b);
void add_ser(char *addr,int port,int c_sock);
void  create_slist(int c_sock);
void * pthread_handler(void *sock);


int main(void)
{
	int i=0,flag=0;
	
	for(i=0;i<num;i++)
		memset(&slist[i],0,sizeof(nodeinfo));
	for(i=0;i<numnode;i++)
		memset(&continuum[i],0,sizeof(mcs));
	
	int s_sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in s_addr;
    memset(&s_addr, 0, sizeof(s_addr));  
    s_addr.sin_family = AF_INET;  
    s_addr.sin_addr.s_addr = inet_addr("127.0.0.1");  
    s_addr.sin_port = htons(LPORT);  
    
    int reuse=1;
    
    if(setsockopt(s_sock,SOL_SOCKET,SO_REUSEADDR,&reuse,sizeof(reuse))==-1)
    {
    	perror("setsockopt");
    	exit(1);
    }
	    
    if(bind(s_sock, (struct sockaddr*)&s_addr, sizeof(s_addr))==-1)
    {
    	perror("bind");
    	exit(1);
    }
	
    if(listen(s_sock, num)==-1)
    {
    	perror("listen");
    	exit(1);
    }
    
    printf("waiting connect...\n");
    
    while(t_num++<num)
    {
    	/*
    	struct timeval time;
    	time.tv_sec=0;
    	time.tv_usec=20;
    	select(0,NULL,NULL,NULL,&time);
    	*/
		int c_sock;
		int msock[10];
		
	   struct sockaddr_in c_addr;
	   socklen_t c_addr_size = sizeof(c_addr);
	   
	   if((c_sock = accept(s_sock, (struct sockaddr*)&c_addr, &c_addr_size))<=0)
	   {
	   		perror("accept");
	   		exit(1);
	   }
	   
	   if(recv(c_sock,&flag,sizeof(int),0)<0)
	   {
			perror("read");
			pthread_exit(0);
		}
		
		if(flag==1)
		{
			char saddr[22]={0};
			int sport=0;
			printf("\nflag:%d  业务服务器\n",flag);
			if(ser_num==1 || ser_num==0)
				msock[ser_num]=c_sock;
			getpeername(c_sock, (struct sockaddr *)&c_addr, &c_addr_size);
			sprintf(saddr,"%s",inet_ntop(AF_INET, &c_addr.sin_addr, ipbuf, sizeof(ipbuf)));
	    	sport=ntohs(c_addr.sin_port);
			add_ser(saddr,sport,c_sock);
			create_continuum(); 
			slist[0].sock=msock[0];
			slist[1].sock=msock[1];
			for(i=0;i<5;i++)
			{
				printf("continuum sock:%d\n",continuum[i].sock);
			}
		}
	   else if(flag==2)
	   {
	   		int * ic_sock = (int *)malloc(sizeof(int)) ;
	   		printf("\nflag:%d  客户端\n",flag);
	   		*ic_sock = c_sock; 
	    	pthread_create(&tid[t_num],NULL,pthread_handler,(void*)ic_sock);
	    	pthread_join(tid[t_num],NULL);
	    	
		}	   
	}
    close(s_sock);
    return 0;
}

void * pthread_handler(void *sock)
{
	int i=0;
	char r_buf1[127]={0};
	char r_buf2[127]={0};
	
	int c_sock=*((int*)sock);
	int ret=0;
	struct sockaddr_in c_addr;
	socklen_t c_addr_size = sizeof(c_addr);
	getpeername(c_sock, (struct sockaddr *)&c_addr, &c_addr_size);
	char saddr[29];
	sprintf(saddr,"%s:%d",inet_ntop(AF_INET, &c_addr.sin_addr, ipbuf, sizeof(ipbuf)),ntohs(c_addr.sin_port));
	ret=find(saddr);
	printf("ret : %d peer addr:%s peer port:%d peer sock:%d\n",ret,continuum[ret].addr,continuum[ret].port,continuum[ret].sock); 
	for(i=0;i<2;i++)
	{
		if(recv(c_sock,r_buf1,127,0)<0)
		{
				perror("read");
				exit(1);
		}
		printf("recv m:  %s\n",r_buf1);
		if(send(continuum[ret].sock,r_buf1,127,0)<0)
		{
				perror("read");
				exit(1);
		}	
		if(recv(continuum[ret].sock,r_buf2,127,0)<0)
		{
				perror("read");
				exit(1);
		}
		printf("recv m:  %s\n",r_buf2);
		if(send(c_sock,r_buf2,127,0)<0)
		{
				perror("read");
				exit(1);
		}	
		printf("第%d次传输完成\n",i+1);
	}
	free(sock);	
	pthread_exit(0);
}
/*
void create_slist(int c_sock)
{
		int i=0;
		int sport,mport[10];
		char saddr[22];
		char maddr[10][22];
		int msock[10];
		struct sockaddr_in c_addr;
		socklen_t c_addr_size = sizeof(c_addr);
		getpeername(c_sock, (struct sockaddr *)&c_addr, &c_addr_size);
	    sprintf(saddr,"%s",inet_ntop(AF_INET, &c_addr.sin_addr, ipbuf, sizeof(ipbuf)));
	    sport=ntohs(c_addr.sin_port);
		strcpy(maddr[ser_num],saddr);
		mport[ser_num]=sport;
		msock[ser_num]=c_sock;
		add_ser(saddr,sport,c_sock);
		for(i=0;i<ser_num;i++)
		{
			strcpy(slist[i].addr,maddr[i]);
			slist[i].port=mport[i];
			slist[i].sock=msock[i];
		}		
}
*/
void add_ser(char *addr,int port,int c_sock)
 {
 	strcpy(slist[ser_num].addr,addr);
	slist[ser_num].port=port;
	slist[ser_num].sock=c_sock;
	ser_num++;
 }
 
//创建环 
int create_continuum(void)
{   
    int  s_num=ser_num;
    int i, k, cont = 0;
    int ks = numservers * 40 / s_num ;  
    for( i = 0; i < s_num; i++ )
    {     
        for( k = 0; k < ks; k++ )
        {
            char ss[32];
            unsigned char digest[16];
            sprintf( ss, "%s:%d:%d", slist[i].addr,slist[i].port, k );
            gethash( ss, digest );
            int h;
            for( h = 0; h < 4; h++ )
            {
                // 把计算出来的连续4位的数字，进行移位。
                continuum[cont].point = ( digest[3+h*4] << 24 )
                                      | ( digest[2+h*4] << 16 )
                                      | ( digest[1+h*4] << 8 )
                                      | digest[h*4];
                memcpy( continuum[cont].addr, slist[i].addr, 22 );
                continuum[cont].port=slist[i].port;
                continuum[cont].sock=slist[i].sock;
                cont++;
            }
        }
    } 
    
    con_num=cont; 
    quicksort(0, con_num );
    return 0; 
}

unsigned int get_hashi( char* inString ) 
{
    unsigned char digest[16];
    gethash( inString, digest );
    return (unsigned int)(( digest[3] << 24 )
                        | ( digest[2] << 16 )
                        | ( digest[1] << 8 )
                        | digest[0] );
}

int  find( char* key) 
{
    unsigned int h = get_hashi( key );
    int i=0;
    for (i=0;i<con_num;i++)
    {
    	if(h>continuum[i].point)
    		continue;
    	else 
    		return i;		
	}
	return 0;
} 
	
void swap(mcs *a,mcs  *b)  
{
    mcs temp;
    temp = *a;
    *a = *b;
    *b = temp;
}

int  quicksort(int begin, int end)
{
    int i, j;
    if(begin < end)
    {
        i = begin + 1;  
        j = end;        
        while(i < j)
        {
            if(continuum[i].point > continuum[begin].point)  
            {
                swap(&continuum[i], &continuum[j]);  
                j--;
            }
            else
            {
                i++;  
            }
        }
        if(continuum[i].point >= continuum[begin].point)  
        {
            i--;
        } 
        swap(&continuum[begin], &continuum[i]);       
        quicksort(begin, i);
        quicksort(j, end);
    }
}	 
