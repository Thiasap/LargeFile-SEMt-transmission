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
	pthread_t thread_id;//��ǰ�߳�id
	int index;//��ǰclients�����±�
	int connectfd; //��ǰ�����׽���
};
typedef struct sFile {
	char mark;
	short threadNum;		//�߳���
	unsigned int spliteSize;//��Ƭ��С��λBytes
	unsigned int filesize;	//�ļ���С
	short useCrypt;			//���ܷ�ʽ
	char filename[255];		//�ļ���
};
#endif