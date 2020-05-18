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
char *sended;	//0未发送，1发送中，2已发送
int splits;		//分块数量
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
	//fclose(sF);
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

//msg_buffer，发送的消息数据，[(4)读取文件位置][(FILE_FRAME_SIZE)文件数据]
void zzsend(int split_index) {
	while(1){	//线程循环使用，递归占内存，新开线程麻烦
		long f_start = split_index * file.spliteSize;	//开始发送的位置
		char buffer[FILE_FRAME_SIZE] = { 0 };
		char msg_buffer[FILE_FRAME_SIZE + INDEX_WRITEFILE_SIZE];
		int pos = 0;
		long nowIndex;//当前读取位置
		unsigned int endSize = file.spliteSize;//剩余应读取的字节
		while (1) {
			memset(buffer, 0, sizeof(buffer));
			memset(msg_buffer, 0, sizeof(msg_buffer));
			nowIndex = f_start + pos;							
			//判断当前分片剩余的空间是否大于读取大小，如果小于，就读剩下的分片大小就行
			int size = rread_file_frame(f_start + pos, buffer, FILE_FRAME_SIZE < endSize ? FILE_FRAME_SIZE : endSize);
			memcpy(msg_buffer, &nowIndex, sizeof(int));
			strcat(msg_buffer, buffer);
			pos += size;
			if (size == FILE_FRAME_SIZE) {
				zmq_msg_t msg_frame;
				zmq_msg_init_size(&msg_frame, size+ INDEX_WRITEFILE_SIZE);
				memcpy(zmq_msg_data(&msg_frame), msg_buffer, size+INDEX_WRITEFILE_SIZE);
				zmq_msg_send(&msg_frame, requester, ZMQ_SNDMORE);
				zmq_msg_close(&msg_frame);
				updateTime(size);
			}
			else if (size < FILE_FRAME_SIZE){
				//printf("[client]read file content over %d bytes\n", pos);
				zmq_msg_t msg_frame;
				zmq_msg_init_size(&msg_frame, size);
				memcpy(zmq_msg_data(&msg_frame), buffer, size);
				zmq_msg_send(&msg_frame, requester, 0);
				zmq_msg_close(&msg_frame);
				updateTime(size);
				//当前分片读完了，标记一下
				pthread_mutex_lock(sended_lock);
				sended[split_index] = 2;
				//寻找下一个未发送的分片
				for (int i = file.threadNum; i < splits; i++) {
					if (sended[i] == 0) {
						split_index = i;
						sended[i] = 1;
						pthread_mutex_unlock(sended_lock);
						break;	//进行下一个循环
					}
					//所有分片已发送
					if (i == splits - 1) return;
				}
			}
		}
		//一个分片发送完了
		printf("[client]%d send end\n",split_index);
	}
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
	//等线程结束
	pthread_exit(NULL);
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
	splits = ((file.filesize - file.spliteSize) / file.spliteSize);
	sended = (char *)malloc(splits * sizeof(char));
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
	pthread_mutex_destroy(&sended_lock);
	pthread_mutex_destroy(&printf_lock);
	for (int i = 0; i < 3; i++) {
		pthread_mutex_destroy(&sendsize);
	}
}