#pragma once
#ifndef _FCONFIG_H
#define _FCONFIG_H
#include <pthread.h>
#define FILE_FRAME_SIZE 10240
#define NAMELENTH 20
#define MAXTHREAD 8
#define MAX_FILENAME 255
#define MARK_SIZE 5 //д���ļ�λ����ռ�ֽ�
#define CRYPT_MODES 5 //AES/DES/3DES/RC2/RC4
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