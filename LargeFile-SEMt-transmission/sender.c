#pragma pack(1)
#include <zmq.h>
#include <stdio.h>
#include"zhelpers.h"
#include"client.h"
#include"fConfig.h"
#define ARGV "test.txt"
FILE* sF;
void *context;
struct sFile file;
char *sended;	//0未发送，1发送中，2已发送
int splits;		//分块数量
double time_start, markTime;
int send_kmg[3];//0,kB,1,MB,2,GB
pthread_mutex_t sended_lock;
pthread_t *pt;
int thread_lock;
int *ptid;
void cupdateTime(int size) {
	send_kmg[0] += size / 1024;
	if (send_kmg[0] >= 1024) {
		send_kmg[1]++;
		send_kmg[0] = send_kmg[0] - 1024;
	}
	if (send_kmg[1] >= 1024) {
		send_kmg[2]++;
		send_kmg[1] = send_kmg[1] - 1024;
	}
	clock_t now = clock();
	double time_now = (double)now / CLOCKS_PER_SEC;
	if (time_now - markTime >= 0.001) {
		printf("[client]%dG %dM %dK sended\n", send_kmg[2], send_kmg[1], send_kmg[0]);
		markTime = time_now;
	}
}
int read_file_frame(char* filename, int pos, char* buffer, int length) {
	fseek(sF, pos, SEEK_SET);
	int size = fread(buffer, 1, length, sF);
	if (size != length) {
		printf("[client]read over!\n");
	}
	//fclose(sF);
	return size;
}
//void zsend(void *se) {
void zsend(int thread_index) {
	short p_id = thread_index;
	//  发送结果的套接字
	void *sender = zmq_socket(context, ZMQ_REQ);
	zmq_connect(sender, "tcp://127.0.0.1:5558");
	filebuf fbuf;
	fbuf.mark = 0;
	while(1){
		char buffer[FILE_FRAME_SIZE] = { 0 };
		int pos = thread_index * file.spliteSize;
		//本线程剩余分片大小
		int i_e = (file.filesize - pos) > file.spliteSize ? file.spliteSize : (file.filesize - pos);
		//下次读取大小,本次分片大小小于默认大小那直接读分片大小
		int next_readS;
		fbuf.splitEnd = 0;
		char flag = 0;
		//Sleep(3000);
		while (1) {
			memset(buffer, 0, sizeof buffer);
			int size;
			next_readS = FILE_FRAME_SIZE < i_e ? FILE_FRAME_SIZE : i_e;
			size = read_file_frame("twly.png", pos, buffer, next_readS);
			//if(thread_index<2)
			//	printf("[client]\t\t\t\t\t\t\t threadindex=%d pos=%d i_e=%d next=%d\n",thread_index,pos,i_e,next_readS);
			fbuf.index = pos;
			memcpy((fbuf.buff), buffer, size);
			//printf("\t\t\t\t\t\t\tss %d %d %d %d pos ftell %d pos %d\n", fbuf.buff[0], fbuf.buff[1], fbuf.buff[2], fbuf.buff[3], pos, fbuf.index);
			pos += size;
			//减去已读取的大小
			i_e -= size;
			if (size == FILE_FRAME_SIZE) {
				fbuf.size = size;
				zmq_send(sender, &fbuf, sizeof(filebuf), 0);
				s_recv(sender);
				cupdateTime(size);
			}
			else if (size < FILE_FRAME_SIZE){
				//当前分片读完了，标记一下
				pthread_mutex_lock(&sended_lock);
				sended[thread_index] = 2;
				//寻找下一个未发送的分片
				flag = 0;	//1是当前分片读完了读下一个分片，2是退出线程
				for (int i = file.threadNum; i < splits; i++) {
					if (sended[i] == 0) {
						thread_index = i;
						sended[i] = 1;
						flag = 1;
						break;//跳出for
					}
					//所有分片已发送，告诉接受端结束传输并写入文件
					if (i == splits - 1) {
						thread_lock--;
						if (thread_lock == 0) fbuf.mark = 1;	//mark会让接收端接受完了直接退出
						flag = 2;
					}
				}
				pthread_mutex_unlock(&sended_lock);
				printf("[client]sended %d bytes\n", pos);
				fbuf.size = size;
				fbuf.splitEnd = 1;
				int rc=zmq_send(sender, &fbuf, sizeof(filebuf), 0); 
				//if (pos == 245760) {
				//	printf("pos\n");
				//}
				//printf("          rc %d", rc);
				char *p = s_recv(sender);
				//int rsize = (int)*p;
				cupdateTime(size);
				if (flag > 0) {
					break;	//进行下一个循环
				}
			}
			//分片结束，线程还没退
		}
		if (flag == 2) {
			printf("[client]Thread %d has done all, will exit!\n", p_id);
			while (thread_lock == 0) {
				s_send(sender, "wait");
				printf("[client]Last thread waiting...\n");
				char *p = s_recv(sender);
				if (NULL!=p&&strcmp(p, "all recv") == 0) break;
				else Sleep(120);
			}
			zmq_close(sender);
			//printf("[client]send end\n");
			return 0;
		}
		//线程退出
	}
}
int sender_init() {
	//发送的数据大小初始化
	for (int i = 0; i < 3; i++) {
		send_kmg[i] = 0;
	}

	context = zmq_ctx_new();
	void *sender = zmq_socket(context, ZMQ_REQ);
	zmq_connect(sender, "tcp://127.0.0.1:5558");
#if 1		//文件信息定义
	char FILENAME[] = "kernel";
	short THREADNUM = 5;
	short useCrypt = 0;
	//int splitSize=splitTest();
	int splitSize = 40960;
#endif // 1
	sF = fopen(FILENAME, "rb");
	if (NULL == sF) {
		printf("[client]fopen failed!\n");
		return 0;
	}
	//获取文件大小
	fseek(sF, 0, SEEK_END);
	unsigned int FILESIZE = ftell(sF);
	fseek(sF, 0, SEEK_SET);
	//初始化file信息
	strcpy(file.filename, FILENAME);
	file.threadNum = THREADNUM;
	file.filesize = FILESIZE;
	file.useCrypt = useCrypt;
	file.spliteSize = splitSize;
	file.mark = 111;
	//计算分块并标记为未发送（0）
	splits = ((file.filesize + file.spliteSize - 1) / file.spliteSize);
	//printf("filesize = %d splitesize = %d splits num=%d\n", file.filesize, file.spliteSize,splits);
	sended = (char *)malloc(splits * sizeof(char));
	memset(sended, 0, splits);
	//初始化线程
	pt = (pthread_t *)malloc(file.threadNum * sizeof(pthread_t));
	ptid = (int *)malloc(file.threadNum * sizeof(int));
	//发送文件信息
	//request
	zmq_send(sender, &file, sizeof(file), 0);
	//recv reply
	char * replay = s_recv(sender);
	printf("[client]server reply: %s\n",replay);
	return 1;
}
void allsend() {
	//初始化一下参数
	if (sender_init() == 0) {
		printf("connect error!\nexit!\n");
		return 0;
	}
	Sleep(3000);
	pthread_mutex_init(&sended_lock, NULL);
	thread_lock = file.threadNum;
	//获取当前时间
	clock_t start;
	start = clock();
	double time_start = (double)start / CLOCKS_PER_SEC;
	markTime = time_start;
	//马上开始线程
	for (int mthread = 0; mthread < file.threadNum; mthread++) {
		ptid[mthread] = pthread_create(&pt[mthread], NULL, zsend, mthread);
		pthread_join(pt[mthread], NULL);
	}
	/*start_end *se1 = (start_end *)malloc(sizeof(start_end));
	start_end *se2 = (start_end *)malloc(sizeof(start_end));
	se1->start = 0; 
	se1->end = 50000;
	se2->start = 50000; 
	se2->end = -1;


	pthread_t aa, qq;
	int ret = pthread_create(&aa, NULL, zsend, (void *)se1);
	if (ret != 0)
	{
		printf("pthread_create error: error_code=%d", ret);
	}
	int ret2 = pthread_create(&qq, NULL, zsend, (void *)se2);
	if (ret2 != 0)
	{
		printf("pthread_create error: error_code=%d", ret2);
	}*/
	Sleep(5000);
	zmq_ctx_destroy(context);
	fclose(sF);
	return 0;
}