#pragma once
#ifndef _FCONFIG_H
#define _FCONFIG_H
#include <pthread.h>
#define FILE_FRAME_SIZE 10240
#define NAMELENTH 20
#define MAXTHREAD 8
struct info
{
	pthread_t thread_id;//��ǰ�߳�id
	int index;//��ǰclients�����±�
	int connectfd; //��ǰ�����׽���
};
#endif