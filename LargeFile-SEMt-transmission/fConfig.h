#pragma once
#ifndef _FCONFIG_H
#define _FCONFIG_H
#include <pthread.h>
#define FILE_FRAME_SIZE 10240
#define NAMELENTH 20
#define MAXTHREAD 8
#define MAX_FILENAME 255

#pragma comment(lib,"pthreadVC2.lib")
struct info
{
	pthread_t thread_id;//当前线程id
	int index;//当前clients数组下表
	int connectfd; //当前连接套接字
};
typedef struct sFile {
	char mark;
	short threadNum;		//线程数
	unsigned int spliteSize;//分片大小单位Bytes
	unsigned int filesize;	//文件大小
	short useCrypt;			//加密方式
	char filename[255];		//文件名
};
#endif