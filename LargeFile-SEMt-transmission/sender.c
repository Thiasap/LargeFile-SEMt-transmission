#pragma pack(1)
#include"client.h"
#include"Cryptions.h"
FILE* sF;
void *context;
struct sFile file;
char *sended;	//0未发送，1发送中，2已发送
int splits;		//分块数量
LARGE_INTEGER start_time, mark_time,CPU_fre, sended_time, recv_time;
int send_kmg[3];//0,kB,1,MB,2,GB
pthread_mutex_t sended_lock,readfile;
pthread_t *pt;
int thread_lock;
int *ptid;
char pwd[65];	//密码
char Recv_all;
char *pAddr;

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
	LARGE_INTEGER now_time;
	QueryPerformanceCounter(&now_time);
	double time_pass = ((double)now_time.QuadPart - (double)mark_time.QuadPart) / (double)CPU_fre.QuadPart;
	if ( time_pass >=500) {
		//printf("[client]%dG %dM %dK sended\n", send_kmg[2], send_kmg[1], send_kmg[0]);
		mark_time = now_time;
	}
}
int read_file_frame(char* filename, int pos, char* buffer, int length) {
	pthread_mutex_lock(&readfile);
	fseek(sF, pos, SEEK_SET);
	int size = fread(buffer, 1, length, sF);
	pthread_mutex_unlock(&readfile);
	if (size != length) {
		printf("[client]read over!\n");
	}
	//fclose(sF);
	return size;
}
void pri(int q) {
	printf("%d\n", q);
	
}
void zsend(int thread_index) {
	//Sleep(5000);
	//return;
	short p_id = thread_index;
	//  发送结果的套接字
	void *sender = zmq_socket(context, ZMQ_REQ);
	zmq_connect(sender, pAddr);
	filebuf fbuf;
	fbuf.mark = 0;
	while (1) {
		char buffer[FILE_FRAME_SIZE] = { 0 };
		int pos = thread_index * file.spliteSize;
		if (pos > file.filesize) {
			printf("[client]don't need me\n");
			return;
		}
		//本线程剩余分片大小
		int i_e = (file.filesize - pos) > file.spliteSize ? file.spliteSize : (file.filesize - pos);
		//下次读取大小,本次分片大小小于默认大小那直接读分片大小
		int next_readS;
		fbuf.splitEnd = 0;
		char flag = 0;
		//Sleep(3000);
		while (1) {
			memset(buffer, 0, sizeof (buffer));
			int size;
			next_readS = FILE_FRAME_SIZE < i_e ? FILE_FRAME_SIZE : i_e;
			size = read_file_frame(file.filename, pos, buffer, next_readS);
			fbuf.size = size;
			fbuf.index = pos;
			if (file.useCrypt != 0) {
				crypt_buffer cb;
				memcpy(cb.buff, buffer, size);
				cb.size = size;
				//printf("before encrypt: %s\n",cb.buff);
				//phex(cb.buff);
				Encrypt(&cb, pwd,file.useCrypt);/*
				printf("after encrypt: \n");
				phex(cb.buff);*/
				memcpy(fbuf.buff, cb.buff, cb.size);
				fbuf.size = cb.size;
			}
			else memcpy((fbuf.buff), buffer, size);
			
			pos += size;
			//减去已读取的大小
			i_e -= size;
			if (size == FILE_FRAME_SIZE) {
				zmq_send(sender, &fbuf, sizeof(filebuf), 0);
				s_recv(sender);
				cupdateTime(size);
			}
			else if (size < FILE_FRAME_SIZE) {
				//当前分片读完了，标记一下
				pthread_mutex_lock(&sended_lock);
				sended[thread_index] = 2;
				//寻找下一个未发送的分片
				flag = 0;	//1是当前分片读完了读下一个分片，2是退出线程
				int i = file.threadNum;
				for (i; i < splits; i++) {
					if (sended[i] == 0) {
						thread_index = i;
						sended[i] = 1;
						flag = 1;
						break;//跳出for
					}
				}
				//所有分片已发送，告诉接受端结束传输并写入文件
				if (i == splits) {
					thread_lock--;
					if (thread_lock == 0)
						fbuf.mark = 1;	//mark会让接收端接受完了直接退出
					QueryPerformanceCounter(&sended_time);
					flag = 2;

				}
				pthread_mutex_unlock(&sended_lock);
				printf("[client]sended %d bytes\n", pos);
				//fbuf.size = size;
				fbuf.splitEnd = 1;
				int rc = zmq_send(sender, &fbuf, sizeof(filebuf), 0);
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
			//printf("[client]Thread %d has done all, will exit!\n", p_id);
			while (thread_lock == 0 && Recv_all != 1) {
				s_send(sender, "wait");
				//printf("[client]Last thread waiting...\n");
				char *p = s_recv(sender);
				if (NULL != p && strcmp(p, "all recv") == 0) {
					Recv_all = 1;
					QueryPerformanceCounter(&recv_time);
					break;
				}
				else Sleep(120);
			}
			zmq_close(sender);
			//printf("[client]send end\n");
			return 0;
		}
		//线程退出
	}
}
int sender_init(argv_info *info) {
	//Sleep(3000);
	//发送的数据信息大小初始化
	for (int i = 0; i < 3; i++) {
		send_kmg[i] = 0;
	}
	char protocol[] = "tcp://";
	int pAddrSize = strlen(protocol) + strlen(info->ip) + strlen(info->port)+2;
	pAddr = (char *)malloc(pAddrSize);
	memset(pAddr, 0, pAddrSize);
	memcpy(pAddr, protocol, strlen(protocol) + 1);
	strcat(pAddr, info->ip);
	pAddr[strlen(pAddr)] = ':';
	pAddr[strlen(pAddr)+1] = 0;
	strcat(pAddr, info->port);

	//  用于发送开始信号的套接字
	void *file_sender;
	if (NULL==(context = zmq_ctx_new()))
		return 0;
	if (NULL==(file_sender = zmq_socket(context, ZMQ_REQ))) {
		zmq_ctx_destroy(context);
		printf("zmq init error!\n");
		return;
	}
	//context = zmq_ctx_new();
	//printf("[client]Connecting %s ...\n", pAddr);
	zmq_connect(file_sender, pAddr);
	memcpy(file.filename, info->filename, strlen(info->filename) + 1);
	file.threadNum = info->threadnum;
	file.useCrypt = info->crypt_mode;
	if (file.useCrypt != 0) {
		memcpy(pwd, StrSHA256(info->passwd, strlen(info->passwd)),65);
	}
	char *fullpath = malloc(strlen(info->filepath) + strlen(info->filename) + 1);
	memcpy(fullpath, info->filepath, strlen(info->filepath) + 1);
	strcat(fullpath, file.filename);
	sF = fopen(fullpath, "rb");
	if (NULL == sF) {
		printf("[client]fopen \"%s\" failed!\n",file.filename);
		return;
	}
	//获取文件大小
	fseek(sF, 0, SEEK_END);
	int FILESIZE = ftell(sF);
	fseek(sF, 0, SEEK_SET);
	if (FILESIZE == 0) {
		printf("[client]read file size error!\n");
		return;
	}
	//初始化file信息
	if((info->SplitSize) == 0)
		file.spliteSize = 40960;
	else {
		int max = getMem();
		file.spliteSize = info->SplitSize< max ? info->SplitSize: max;
	}
	file.filesize = FILESIZE;
	file.mark = 111;
	//计算分块并标记为未发送（0）
	splits = ((file.filesize + file.spliteSize - 1) / file.spliteSize);
	file.threadNum = splits < file.threadNum ? splits : file.threadNum;
	//printf("filesize = %d splitesize = %d splits num=%d\n", file.filesize, file.spliteSize,splits);

	sended = (char *)malloc(splits * sizeof(char));
	memset(sended, 0, splits);
	//初始化线程
	pt = (pthread_t *)malloc(file.threadNum * sizeof(pthread_t));
	ptid = (int *)malloc(file.threadNum * sizeof(int));
	//发送文件信息
	//request
	zmq_send(file_sender, &file, sizeof(file), 0);
	//recv reply
	char * replay = s_recv(file_sender);
	if (replay == "") return;
	//printf("[client]server reply: %s\n",replay);
	Recv_all = 0;
	return 1;
}
void allsend(argv_info *info) {
	//初始化一下参数
	if (sender_init(info)==0) {
		printf("init failed!\n");
		return 0;
	}
	pthread_mutex_init(&sended_lock, NULL); 
	pthread_mutex_init(&readfile, NULL); 
	thread_lock = file.threadNum;
	//获取当前时间
	QueryPerformanceFrequency(&CPU_fre);
	QueryPerformanceCounter(&start_time);
	mark_time = start_time;

	//马上开始线程
	for (int mthread = 0; mthread < file.threadNum; mthread++) {
		ptid[mthread] = pthread_create(&pt[mthread], NULL, zsend, mthread);
		pthread_join(pt[mthread], NULL);
	}
	printf("%d %f %f\n", file.filesize,
		((double)sended_time.QuadPart - (double)start_time.QuadPart) / (double)CPU_fre.QuadPart * 1000,
		((double)recv_time.QuadPart - (double)start_time.QuadPart) / (double)CPU_fre.QuadPart * 1000);
	zmq_ctx_destroy(context);
	fclose(sF);
	return 0;
}