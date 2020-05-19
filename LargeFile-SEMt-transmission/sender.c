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
char *sended;	//0δ���ͣ�1�����У�2�ѷ���
int splits;		//�ֿ�����
double time_start, markTime;
int send_kmg[3];//0,kB,1,MB,2,GB
pthread_t *pt;
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
	//  ���ͽ�����׽���
	void *sender = zmq_socket(context, ZMQ_PUSH);
	zmq_connect(sender, "tcp://127.0.0.1:5558");
	filebuf fbuf;
	fbuf.mark = 0;
	while(1){
		char buffer[FILE_FRAME_SIZE] = { 0 };
		int pos = thread_index * file.spliteSize;
		//���߳�ʣ���Ƭ��С
		int i_e = (file.filesize - pos) > file.spliteSize ? file.spliteSize : (file.filesize - pos);
		//�´ζ�ȡ��С,���η�Ƭ��СС��Ĭ�ϴ�С��ֱ�Ӷ���Ƭ��С
		int next_readS;
		//Sleep(3000);
		while (1) {
			memset(buffer, 0, sizeof buffer);
			int size;
			next_readS = FILE_FRAME_SIZE < i_e ? FILE_FRAME_SIZE : i_e;
			size = read_file_frame("twly.png", pos, buffer, next_readS);
			fbuf.index = pos;
			memcpy((fbuf.buff), buffer, size);
			pos += size;
			//��ȥ�Ѷ�ȡ�Ĵ�С
			i_e -= size;
			if (size == FILE_FRAME_SIZE) {
				fbuf.size = size;
				zmq_send(sender, &fbuf, sizeof(filebuf), 0);
				//cupdateTime(size);
			}
			else if (size < FILE_FRAME_SIZE){
				//��ǰ��Ƭ�����ˣ����һ��
				sended[thread_index] = 2;
				//Ѱ����һ��δ���͵ķ�Ƭ
				char flag = 0;
				for (int i = file.threadNum; i < splits; i++) {
					if (sended[i] == 0) {
						thread_index = i;
						sended[i] = 1;
						flag = 1;
						break;//����for
					}
					//���з�Ƭ�ѷ��ͣ����߽��ܶ˽������䲢д���ļ�
					if (i == splits - 1) {
						fbuf.mark = 1;
						flag = 1;
					}
				}
				printf("[client]this index content over %d bytes\n", pos);
				fbuf.size = size;
				zmq_send(sender, &fbuf, sizeof(filebuf), 0); 
				cupdateTime(size);
				if (flag == 1) {
					flag = 0;
					break;	//������һ��ѭ��
				}
			}
		}
		if (fbuf.mark == 1) {
			Sleep(8000);
			zmq_close(sender);
			printf("[client]send end\n");
			return 0;
		}
	}
}
int sender_init() {
	//���͵����ݴ�С��ʼ��
	for (int i = 0; i < 3; i++) {
		send_kmg[i] = 0;
	}

	context = zmq_ctx_new();
	void *sender = zmq_socket(context, ZMQ_PUSH);
	zmq_connect(sender, "tcp://127.0.0.1:5558");
#if 1		//�ļ���Ϣ����
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
	//��ȡ�ļ���С
	fseek(sF, 0, SEEK_END);
	unsigned int FILESIZE = ftell(sF);
	fseek(sF, 0, SEEK_SET);
	//��ʼ��file��Ϣ
	strcpy(file.filename, FILENAME);
	file.threadNum = THREADNUM;
	file.filesize = FILESIZE;
	file.useCrypt = useCrypt;
	file.spliteSize = splitSize;
	file.mark = 111;
	//����ֿ鲢���Ϊδ���ͣ�0��
	splits = ((file.filesize + file.spliteSize - 1) / file.spliteSize);
	//printf("filesize = %d splitesize = %d splits num=%d\n", file.filesize, file.spliteSize,splits);
	sended = (char *)malloc(splits * sizeof(char));
	memset(sended, 0, splits);
	//��ʼ���߳�
	pt = (pthread_t *)malloc(file.threadNum * sizeof(pthread_t));
	ptid = (int *)malloc(file.threadNum * sizeof(int));
	//�����ļ���Ϣ
	zmq_send(sender, &file, sizeof(file), 0);

	return 1;
}
void allsend() {
	//��ʼ��һ�²���
	if (sender_init() == 0) {
		printf("connect error!\nexit!\n");
		return 0;
	}
	Sleep(3000);
	//��ȡ��ǰʱ��
	clock_t start;
	start = clock();
	double time_start = (double)start / CLOCKS_PER_SEC;
	markTime = time_start;
	//���Ͽ�ʼ�߳�
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