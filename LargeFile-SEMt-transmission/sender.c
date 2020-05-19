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

int read_file_frame(char* filename, int pos, char* buffer, int length) {
	
	fseek(sF, pos, SEEK_SET);
	int size = fread(buffer, 1, length, sF);
	if (size != length) {
		printf("[client]read over!\n");
	}
	//fclose(sF);
	return size;
}
void zsend(void *se) {

	//  发送结果的套接字
	void *sender = zmq_socket(context, ZMQ_PUSH);
	zmq_connect(sender, "tcp://127.0.0.1:5558");
	char buffer[FILE_FRAME_SIZE] = { 0 };
	int pos = 0;
	start_end *s_e = (start_end *)se;
	pos = s_e->start;
	int ssindex = s_e->end == -1 ? 0 : s_e->end - pos;
	Sleep(3000);
	int sendedk = 0;
	int sendedm = 0;
	int sendedg = 0;
	filebuf fbuf;
	fbuf.mark = 0;
	while (1) {
		memset(buffer, 0, sizeof buffer);
		int size;
		size = read_file_frame("twly.png", pos, buffer,(s_e->end == -1||FILE_FRAME_SIZE < ssindex)? FILE_FRAME_SIZE:ssindex);
		fbuf.index = pos;
		memcpy((fbuf.buff), buffer, size);
		pos += size;
		ssindex -= size;
		if (size == FILE_FRAME_SIZE) {
			fbuf.size = size;
			zmq_send(sender, &fbuf, sizeof(filebuf), 0);
			sendedk += size / 1024;
			if (sendedk >= 1024) {
				sendedm++;
				sendedk = sendedk - 1024;
			}if (sendedm >= 1024) {
				sendedg++;
				sendedm = sendedm - 1024;
			}
			printf("[client]%dG %dM %dK sended\n", sendedg, sendedm, sendedk);
		}
		else if (size < FILE_FRAME_SIZE)
		{
			if(pos>60000) fbuf.mark = 1;
			printf("[client]read file content over %d bytes\n", pos);
			fbuf.size = size;
			zmq_send(sender, &fbuf, sizeof(filebuf), 0);
			sendedk += size / 1024;
			if (sendedk >= 1024) {
				sendedm++;
				sendedk = sendedk - 1024;
			}if (sendedm >= 1024) {
				sendedg++;
				sendedm = sendedm - 1024;
			}
			printf("[client]%dG %dM %dK sended\n", sendedg, sendedm, sendedk);
			break;
		}
	}
	zmq_close(sender);
	printf("[client]send end\n");
	return 0;
}
int sender_init() {
	context = zmq_ctx_new();
	void *sender = zmq_socket(context, ZMQ_PUSH);
	zmq_connect(sender, "tcp://127.0.0.1:5558");
#if 1
	char FILENAME[] = "twly.png";
	short THREADNUM = 1;
	short useCrypt = 0;
	//int splitSize=splitTest();
	int splitSize = 40960;
#endif // 1
	sF = fopen(FILENAME, "rb");
	if (NULL == sF) {
		printf("[client]fopen failed!\n");
		return 0;
	}
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
	splits = ((file.filesize - file.spliteSize) / file.spliteSize + 1);
	//printf("filesize = %d splitesize = %d splits num=%d\n", file.filesize, file.spliteSize,splits);
	sended = (char *)malloc(splits * sizeof(char));
	memset(sended, 0, splits);
	zmq_send(sender, &file, sizeof(file), 0);
	char *replayName= (char *)malloc(splits * sizeof(file.filename));
	//char *replayName = s_recv(sender);
	//memcpy(replayName, s_recv(), sizeof(s_recv));
	memset(replayName, 0, sizeof(file.filename));
	Sleep(3000);
	return 1;
	/*int size = zmq_recv(sender, replayName, sizeof(file.filename), 0);
	if (replayName == NULL) return 0;
	printf("[client]recv reply file name: %s\n", replayName);
	if (strcmp(file.filename, replayName) == 1)
		return 1;
	else return 0;*/
}
void allsend() {
	if (sender_init() == 0) {
		printf("connect error!\nexit!\n");
		return 0;
	}
	Sleep(1000);

	start_end *se1 = (start_end *)malloc(sizeof(start_end));
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
	}
	Sleep(5000);
	zmq_ctx_destroy(context);
	fclose(sF);
	return 0;
}