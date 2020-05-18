// LargeFile-SEMt-transmission.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//
#include <zmq.h>
#include <pthread.h>
#include "zhelpers.h"
//#include "server.h"
//#include "client.h"
#include"fConfig.h"
#include"clientSend.h"



int main() {
	//testrouter();
	pthread_t pSend, pRecv,pRouter;
	//int ret = pthread_create(&pSend, NULL, zsend, NULL);
	int ret = pthread_create(&pSend, NULL, rrbroker, NULL);
	if (ret != 0)
	{
		printf("pthread_create error: error_code=%d", ret);
	}
	//int ret2 = pthread_create(&pRecv, NULL, zrecv, NULL);
	/*int ret2 = pthread_create(&pRecv, NULL, rrworker, NULL);
	if (ret2 != 0)
	{
		printf("pthread_create error: error_code=%d", ret2);
	}*/
	int ret3 = pthread_create(&pRouter, NULL, rrclient, NULL);
	if (ret3 != 0)
	{
		printf("pthread_create error: error_code=%d", ret3);
	}
	pthread_exit(NULL);
}
