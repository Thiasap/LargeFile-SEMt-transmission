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

	//�����ļ�ָ��
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
	//ʹ��tcpЭ�����ͨ�ţ���Ҫ���ӵ�Ŀ�����IP��ַΪ192.168.1.2
	//ͨ��ʹ�õ�����˿� Ϊ7766 
	const char * pAddr = "tcp://127.0.0.1:7766";

	//����context 
	if ((pCtx = zmq_ctx_new()) == NULL)
	{
		return 0;
	}
	//����socket 
	if ((pSock = zmq_socket(pCtx, ZMQ_DEALER)) == NULL)
	{
		zmq_ctx_destroy(pCtx);
		return 0;
	}
	int iSndTimeout = 5000;// millsecond
	//���ý��ճ�ʱ 
	if (zmq_setsockopt(pSock, ZMQ_RCVTIMEO, &iSndTimeout, sizeof(iSndTimeout)) < 0)
	{
		zmq_close(pSock);
		zmq_ctx_destroy(pCtx);
		return 0;
	}
	//����Ŀ��IP192.168.1.2���˿�7766 
	if (zmq_connect(pSock, pAddr) < 0)
	{
		zmq_close(pSock);
		zmq_ctx_destroy(pCtx);
		return 0;
	}
	return 0;
}