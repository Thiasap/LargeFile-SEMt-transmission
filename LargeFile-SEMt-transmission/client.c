#include <zmq.h>
#include "stdio.h"
#include"client.h"
#include"fConfig.h"
#define _PRO "[client]"
struct file_route_msg {
	unsigned int path_len;
	char path[255];
};

int read_file_frame(char* filename, int pos, char* buffer, int length) {
	FILE* fp = fopen(filename, "rb");
	if (NULL == fp) {
		printf(_PRO, "fopen failed!\n");
		return -1;
	}

	//调整文件指针
	fseek(fp, pos, SEEK_SET);
	int size = fread(buffer, 1, length, fp);
	if (size != length) {
		printf(_PRO, "read over!\n");
	}
	fclose(fp);
	return size;
}

int zsend()
{
	void * pCtx = NULL;
	void * pSock = NULL;
	//使用tcp协议进行通信，需要连接的目标机器IP地址为192.168.1.2
	//通信使用的网络端口 为7766 
	const char * pAddr = "tcp://127.0.0.1:7766";

	//创建context 
	if ((pCtx = zmq_ctx_new()) == NULL)
	{
		return 0;
	}
	//创建socket 
	if ((pSock = zmq_socket(pCtx, ZMQ_DEALER)) == NULL)
	{
		zmq_ctx_destroy(pCtx);
		return 0;
	}
	int iSndTimeout = 5000;// millsecond
	//设置接收超时 
	if (zmq_setsockopt(pSock, ZMQ_RCVTIMEO, &iSndTimeout, sizeof(iSndTimeout)) < 0)
	{
		zmq_close(pSock);
		zmq_ctx_destroy(pCtx);
		return 0;
	}
	//连接目标IP192.168.1.2，端口7766 
	if (zmq_connect(pSock, pAddr) < 0)
	{
		zmq_close(pSock);
		zmq_ctx_destroy(pCtx);
		return 0;
	}
	return 0;
}