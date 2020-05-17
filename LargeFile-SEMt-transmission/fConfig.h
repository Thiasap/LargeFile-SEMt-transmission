#pragma once
#ifndef _FCONFIG_H
#define _FCONFIG_H
#include <pthread.h>
#define FILE_FRAME_SIZE 10240
#define NAMELENTH 20
#define MAXTHREAD 8
struct info
{
	pthread_t thread_id;//当前线程id
	int index;//当前clients数组下表
	int connectfd; //当前连接套接字
};
#endif