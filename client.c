#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h> 
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <pthread.h>

#define port 1234
#define num 5

int flag=2;
pthread_t tid[num];

void * pthread_handler(void* id)
{
		int i=0;
    	int sock ;
		int p_id=*((int*)id);
    	if((sock = socket(AF_INET, SOCK_STREAM, 0))==-1)
    	{
        	perror("socket");
        	pthread_exit(0);
    	}	
    	struct sockaddr_in serv_addr;
    	memset(&serv_addr, 0, sizeof(serv_addr));  
    	serv_addr.sin_family = AF_INET;  
    	serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");  
    	serv_addr.sin_port = htons(port); 
    
    	if(connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr))==-1)
    	{
    		perror("connect");
    		pthread_exit(0);
		}
		printf("connect successful\n");
	
		char s_buf[127];
		char r_buf[127];
		memset(s_buf,'0',126);
		sprintf(s_buf,"hello,this is client %d\n",p_id );
	    send(sock,&flag,sizeof(int),0);
   		
   		for(i=0;i<2;i++)
		{
			if(send(sock,s_buf,127,0)<0)
			{
				perror("read");
				pthread_exit(0);
			}	
			else 
				printf("client %d 第%d次发送成功\n",p_id,i+1);
			
			if(recv(sock,r_buf,127,0)<0)
			{
				perror("read");
				pthread_exit(0);
			}
			printf("client %d recv message:%s",p_id,r_buf);	
		}
		free(id); 		
}

int main()
{
	int i = 0;
	int err;
	int *a[5];
	for(i=0;i<num;i++)
	{
		a[i]=malloc(sizeof(int));
		*a[i]=i;
		if(pthread_create(&tid[i],NULL,pthread_handler,a[i])!=0)
		{
			printf("create thread failes\n");	
		}		
		else 
			printf("thread %d  create successful\n",i);
	}
	for(i=0;i<num;i++)
	{
		if((err=pthread_join(tid[i],NULL))!=0)
		{
			printf("thread %d ends failes,%s\n",i,strerror(err));
		}
	} 
        return 0;
}
