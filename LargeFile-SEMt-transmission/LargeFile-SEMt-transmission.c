// LargeFile-SEMt-transmission.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <stdio.h>
#include<pthread.h>
#include"client.h"
#include"server.h"
#include"fConfig.h"
#pragma comment(lib,"pthreadVC2.lib") 
int main() {
	pthread_t pSend, pRecv;
	int ret = pthread_create(&pSend, NULL, zsend, NULL);
	if (ret != 0)
	{
		printf("pthread_create error: error_code=%d", ret);
	}
	int ret2 = pthread_create(&pRecv, NULL, zrecv, NULL);
	if (ret != 0)
	{
		printf("pthread_create error: error_code=%d", ret2);
	}
	pthread_exit(NULL);
}
