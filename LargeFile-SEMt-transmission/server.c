#include <zmq.h>
#include "stdio.h"
#include"server.h"
#include"fConfig.h"

int zrecv()
{
	void * pCtx = NULL;
	void * pSock = NULL;
	const char * pAddr = "tcp://*:7766";

	//创建context，zmq的socket 需要在context上进行创建 
	if ((pCtx = zmq_ctx_new()) == NULL)
	{
		return 0;
	}
	//创建zmq socket ，socket目前有6中属性 ，这里使用dealer方式
	//具体使用方式请参考zmq官方文档（zmq手册） 
	if ((pSock = zmq_socket(pCtx, ZMQ_DEALER)) == NULL)
	{
		zmq_ctx_destroy(pCtx);
		return 0;
	}
	int iRcvTimeout = 5000;// millsecond
	//设置zmq的接收超时时间为5秒 
	if (zmq_setsockopt(pSock, ZMQ_RCVTIMEO, &iRcvTimeout, sizeof(iRcvTimeout)) < 0)
	{
		zmq_close(pSock);
		zmq_ctx_destroy(pCtx);
		return 0;
	}
	//绑定地址 tcp://*:7766 
	//也就是使用tcp协议进行通信，使用网络端口 7766
	if (zmq_bind(pSock, pAddr) < 0)
	{
		zmq_close(pSock);
		zmq_ctx_destroy(pCtx);
		return 0;
	}
	printf("bind at : %s\n", pAddr);

	FILE *recvF;
	recvF = fopen("recv.txt", "wb");
	if (recvF==NULL)
	{
		printf("stop because open file err");
		return 0;
	}
	remove(recvF);
	unsigned char buf[MAXLEN];
	unsigned char stopFlag[MAXLEN] = "end";
	int time = 0;
	zmq_msg_t recvMsg;
	zmq_recv(pSock, &recvMsg);
	while (1) {
		time++;
		printf("[s]time=%d  \n", time);
		errno = 0;
		if (zmq_recv(pSock, buf, sizeof(buf), 0) < 0)
		{
			printf("error = %s\n", zmq_strerror(errno));
			continue;
		}
		else {
			if (strcmp(buf, stopFlag) == 0) break;
			fwrite(buf, sizeof(unsigned char), 20, recvF);
		}
		if (time==20)
		{
			printf("stop because 6 times\n");
			break;
		}
	}
	printf("end");
	fclose(recvF);
	return 0;
	while (1)
	{
		char szMsg[1024] = { 0 };
		printf("waitting...\n");
		errno = 0;
		//循环等待接收到来的消息，当超过5秒没有接到消息时，
		//zmq_recv函数返回错误信息 ，并使用zmq_strerror函数进行错误定位 
		
		if (zmq_recv(pSock, szMsg, sizeof(szMsg), 0) < 0)
		{
			printf("error = %s\n", zmq_strerror(errno));
			continue;
		}
		printf("received message : %s\n", szMsg);
	}

	return 0;
}