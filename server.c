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

int flag=1;
pthread_t tid[num];

void * pthread_handler(void* id)
{
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
	
		char s_buf[127];
		char r_buf[127];
		memset(s_buf,'0',126);
		sprintf(s_buf,"hello,this is server %d\n",p_id );
	    send(sock,&flag,sizeof(int),0);
	    while(1)
	    {
	    	recv(sock,r_buf,127,0);
	    	printf("r_buf:%s\n",r_buf);
   			send(sock,s_buf,strlen(s_buf),0);
		}
    	pthread_exit(0);
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
