// LargeFile-SEMt-transmission.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//
#include <zmq.h>
#include <pthread.h>
#include "zhelpers.h"
#include "server.h"
#include<czmq.h>
#include "client.h"
#include"fConfig.h"
//#include"clientSend.h"
//#include "serverRecv.h"

int main() {
	//testrouter();
	pthread_t pSend, pRecv,pRouter;
	int ret3 = pthread_create(&pRouter, NULL, zrecv, NULL);
	if (ret3 != 0)
	{
		printf("pthread_create error: error_code=%d", ret3);
	}
	allsend();
	//int ret = pthread_create(&pSend, NULL, zsend, NULL);
	/*int ret = pthread_create(&pSend, NULL, allsend, NULL);
	if (ret != 0)
	{
		printf("pthread_create error: error_code=%d", ret);
	}*/
	//int ret2 = pthread_create(&pRecv, NULL, zrecv, NULL);
	/*int ret2 = pthread_create(&pRecv, NULL, zsend, (void *)se1);
	if (ret2 != 0)
	{
		printf("pthread_create error: error_code=%d", ret2);
	}*/

	pthread_exit(NULL);
}
