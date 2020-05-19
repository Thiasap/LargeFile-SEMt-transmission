#pragma once
#ifndef _FCONFIG_H
#define _FCONFIG_H
#include <pthread.h>
#define FILE_FRAME_SIZE 10240
#define NAMELENTH 20
#define MAXTHREAD 8
#define MAX_FILENAME 255
#define MARK_SIZE 5 //写入文件位置所占字节
#define CRYPT_MODES 5 //AES/DES/3DES/RC2/RC4
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
typedef struct packInfo {
	short isAllend;
	long index;
	int size;

};
typedef struct filebuf {
	long index;
	short mark;
	int size;
	char splitEnd;
	char buff[FILE_FRAME_SIZE];
}filebuf;
typedef struct start_end {
	long start;
	long end;
}start_end;
typedef struct argv_info {
	char work_mode;		//-r
	char ip[16];		//-l
	char port[6];		//-p
	int crypt_mode;		//-c
	char passwd[33];	//-k
	char filename[255];	//-f
}argv_info;
#endif