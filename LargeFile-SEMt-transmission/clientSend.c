#pragma pack(1)
#include <zmq.h>
#include <stdio.h>
#include <malloc.h>
#include <time.h>
#include "fConfig.h"
#include "clientSend.h"
#include "zhelpers.h"
//typedef struct sFile {
//	short threadNum;		//线程数
//	unsigned int spliteSize;//分片大小单位Bytes
//	unsigned int filesize;	//文件大小
//	short useCrypt;			//加密方式
//	char filename[255];		//文件名
//};

/*void structToChar(struct sFile file, char *p) {
	memset(p, 0, sizeof(file));

}*/
void *context;	//构建zmq上下文
void *requester;
FILE *sF;
struct sFile file;//文件信息
pthread_mutex_t printf_lock;
pthread_mutex_t sended_lock;
char *sended;
double time_start,markTime;
pthread_mutex_t sendsize[3];
int send_kmg[3];//0,kB,1,MB,2,GB
//sender--init_zmq-->rrclient-n->zzsend->rread_file_frame
int rread_file_frame(int pos, char* buffer, int length) {
	/*FILE* fp = fopen(filename, "rb");
	if (NULL == fp) {
		printf("[client]fopen failed!\n");
		return -1;
	}*/
	fseek(sF, pos, SEEK_SET);
	int size = fread(buffer, 1, length, sF);
	if (size != length) {
		printf("[client]read over!\n");
	}
	fclose(sF);
	return size;
}
int init_sender() {
	if ((context = zmq_ctx_new()) == NULL)
		return 0;
	if ((requester = zmq_socket(context, ZMQ_REQ)) == NULL) {
		zmq_ctx_destroy(context);
		return 0;
	}
	if (zmq_connect(requester, "tcp://localhost:5559") < 0) {
		zmq_close(requester);
		zmq_ctx_destroy(context);
		return 0;
	}
	char *pf = (char *)malloc(sizeof(file));
	memcpy(pf, (char *)&file, sizeof(file));
	zmq_send(requester, pf, sizeof(file), 0);
	char *replayName = s_recv(requester);
	printf("[client]recv reply file name: %s\n", replayName);
	if (strcmp(file.filename, replayName) == 1)
		return 1;
	else return 0;
}
void updateTime(int size) {
	pthread_mutex_lock(sendsize[0]);
	pthread_mutex_lock(sendsize[1]);
	send_kmg[0] += size / 1024;
	if (send_kmg[0] >= 1024) {
		send_kmg[1]++;
		send_kmg[0] = send_kmg[0] - 1024;
	}
	pthread_mutex_unlock(sendsize[0]);
	pthread_mutex_unlock(sendsize[1]);
	if (send_kmg[1] >= 1024) {
		pthread_mutex_lock(sendsize[1]);
		pthread_mutex_lock(sendsize[2]);
		send_kmg[2]++;
		send_kmg[1] = send_kmg[1] - 1024;
		pthread_mutex_unlock(sendsize[1]);
		pthread_mutex_unlock(sendsize[2]);
	}
	pthread_mutex_lock(printf_lock);
	clock_t now = clock();
	double time_now = (double)now / CLOCKS_PER_SEC;
	if (time_now - markTime >= 1) {
		printf("[client]%dG %dM %dK sended\n", send_kmg[2], send_kmg[1], send_kmg[0]);
		markTime = time_now;
	}
	pthread_mutex_unlock(printf_lock);
}
void zzsend(int split_index) {
	long f_start = split_index * file.spliteSize;	//开始发送的位置
	char buffer[FILE_FRAME_SIZE] = { 0 };
	int pos = 0;
	while (1) {
		memset(buffer, 0, sizeof buffer);
		int size = rread_file_frame(f_start+pos, buffer, FILE_FRAME_SIZE);
		pos += size;
		if (size == FILE_FRAME_SIZE) {
			zmq_msg_t msg_frame;
			zmq_msg_init_size(&msg_frame, size);
			memcpy(zmq_msg_data(&msg_frame), buffer, size);
			zmq_msg_send(&msg_frame, requester, ZMQ_SNDMORE);
			zmq_msg_close(&msg_frame);/*
			send_kmg[0] += size / 1024;
			if (send_kmg[0] >= 1024) {
				send_kmg[1]++;
				send_kmg[0] = send_kmg[0] - 1024;
			}if (send_kmg[1] >= 1024) {
				send_kmg[2]++;
				send_kmg[1] = send_kmg[1] - 1024;
			}

			printf("[client]%dG %dM %dK sended\n", send_kmg[2], send_kmg[1], send_kmg[0]);*/
			updateTime(size);
		}
		else if (size < FILE_FRAME_SIZE)
		{
			printf("[client]read file content over %d bytes\n", pos);
			zmq_msg_t msg_frame;
			zmq_msg_init_size(&msg_frame, size);
			memcpy(zmq_msg_data(&msg_frame), buffer, size);
			zmq_msg_send(&msg_frame, requester, 0);
			zmq_msg_close(&msg_frame);
			updateTime(size);
			break;
		}
	}
	zmq_close(sender);
	zmq_ctx_destroy(context);
	printf("[client]send end\n");
	return 0;
}
void rrclient(int threadMax) {
	int mthread;
	pthread_t *pt;
	pt = (pthread_t *)malloc(threadMax * sizeof(pthread_t));
	int *ptid;
	ptid = (int *)malloc(threadMax * sizeof(int));
	//获取当前时间
	clock_t start;
	start = clock();
	double time_start = (double)start/ CLOCKS_PER_SEC;
	markTime = time_start;
	//发送的数据大小初始化
	for (int i = 0; i < 3; i++) {
		send_kmg[i] = 0;
		pthread_mutex_init(&sendsize[i], NULL);
	}
	for (mthread = 0; mthread < threadMax; mthread++) {
		ptid[mthread] = pthread_create(&pt[mthread], NULL, zzsend, mthread);
	}
	zmq_close(requester);
	zmq_ctx_destroy(context);
	return 0;
}
void sender() {
	char FILENAME[] = "twly.png";
	short THREADNUM = 4;
	short useCrypt = 0;
	//int splitSize=splitTest();
	int splitSize = 40960;
	sF = fopen(FILENAME, "rb");
	fseek(sF, 0, SEEK_END);
	unsigned int FILESIZE = ftell(sF);
	fseek(sF, 0, SEEK_SET);
	strcpy(file.filename, FILENAME);
	file.threadNum = THREADNUM;
	file.filesize = FILESIZE;
	file.useCrypt = useCrypt;
	file.spliteSize = splitSize;
	file.mark = 111;
	//计算分块并标记为未发送（0）
	sended = (char *)malloc(file.spliteSize * sizeof(char));
	memset(sended, 0, sizeof(sended));
	pthread_mutex_init(&sended_lock, NULL); 
	pthread_mutex_init(&printf_lock, NULL);
	//初始化
	if (init_sender() == 0) {
		printf("connect error!\nexit!\n");
		return 0;
	}
	return;
	rrclient(file.threadNum);
	pthread_exit(NULL);
	pthread_mutex_destroy(&sended_lock);
	pthread_mutex_destroy(&printf_lock);
	for (int i = 0; i < 3; i++) {
		pthread_mutex_destroy(&sendsize);
	}
}