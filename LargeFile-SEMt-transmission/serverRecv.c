#pragma pack(1)
#include "serverRecv.h"
FILE *rF;
struct sFile file;
pthread_mutex_t *file_lock;
pthread_mutex_t *printf_lock;
pthread_mutex_t *sended_lock;	//发送块标记锁
char *sended;					//发送块标记
double time_start, markTime;	//开始时间，上次获得的时间
pthread_mutex_t *recvsize[3];	//已接收数据大小变量锁
int recv_kmg[3];				//已接收数据大小
int threadExit;
int byte2int(char *buf) {
	int a;
	char *tint = &a;
	for (int start = 1; start < 5; start++) {
		tint[start-1] = buf[start];
	}
	return a;
}
int init_recver(void * frontend) {
	char *pf;
	pf = (char *)malloc(sizeof(file));
	zmq_recv(frontend, pf, sizeof(file), 0);
	memcpy((char *)&file, pf, sizeof(file));
	if (file.mark != 111) {
		printf("[server]pf mark: \n");
		for (int i = 0; i < 10; i++) {
			printf("%d ",pf[i]);
		}
		printf("\n[server]init Error! Cannot recv file info");
		return 0;
	}
	s_send(frontend, file.filename);
	printf("[server]recv file name: %s\n", file.filename);
	strcat(file.filename, "_recv");
	if ((rF = fopen(file.filename, "wb")) == NULL){
		printf("[server]stop because open file err\n");
		return 0;
	}
	threadExit = 0;
	//初始化锁
	pthread_mutex_init(file_lock, NULL);
	pthread_mutex_init(sended_lock, NULL);
	pthread_mutex_init(printf_lock, NULL);
	for (int i = 0; i < 3; i++) {
		recv_kmg[i] = 0;
		pthread_mutex_init(&recvsize[i], NULL);
	}
	return 1;
}
void recver(void *context) {
	//void *context = zmq_ctx_new();
	printf("recving...\n");
	//  Socket to talk to clients
	void *responder = zmq_socket(context, ZMQ_REP);
	zmq_bind(responder, "tcp://*:5559");
	//zmq_connect(responder, "inproc://workers");
	zmq_msg_t message;
	char buffer[FILE_FRAME_SIZE] = { 0 };
	char msg_buffer[FILE_FRAME_SIZE + MARK_SIZE];
	long index_fseek=0;
	struct packInfo packinfo;
	char info[sizeof(packinfo)];
	while (1) {
		//zmq_msg_init(&message);
		//zmq_msg_recv(&message, responder, 0);
		zmq_recv(responder, info, sizeof(packinfo), 0);
		memcpy(&packinfo, info, sizeof(packinfo));
		zmq_recv(responder, buffer, packinfo.size, 0);
		printf("recved...\n");
		//int size = zmq_msg_size(&message);
		int size = packinfo.size;
		printf("[server]recv size %d\n", size);
		//memset(msg_buffer, 0, sizeof(msg_buffer));			//初始化msg_buffer
		//memcpy(msg_buffer, zmq_msg_data(&message), size);	//将zmq的数据存入msg_buffer
		//zmq_msg_close(&message);
		//if (msg_buffer[0] == 0) break;
		//index_fseek = byte2int(msg_buffer);
		////memcpy(&index_fseek, msg_buffer, sizeof(long));		//提取index标记
		//if (size == MARK_SIZE) {
		//	int flag = 0;
		//	for (int i = 0; i < MARK_SIZE; i++) {
		//		flag += msg_buffer[i] == -1 ? 1 : 0;
		//	}
		//	if (flag==4){
		//		//pthread_mutex_lock(printf_lock);
		//		printf("[server]Thread exit, because all sended by marksign\n");
		//		return;
		//	}
		//}
		////跳过标记之后就是文件数据
		//memcpy(buffer, msg_buffer+ MARK_SIZE, sizeof(buffer));
		//开始写入
		//pthread_mutex_lock(file_lock);
		index_fseek = packinfo.index;
		fseek(rF, index_fseek, SEEK_SET);
		fwrite(buffer, sizeof(char), sizeof(packinfo.size), rF);
		if (packinfo.isAllend == 1) break;
		//fwrite(buffer, sizeof(char), size- MARK_SIZE, rF);
		//pthread_mutex_unlock(file_lock);
	}
	zmq_close(responder);
	fflush(rF);
	fclose(rF);
	threadExit++;
	return 0;

}
void rrbroker() {
	void *context = zmq_ctx_new();
	void *frontend = zmq_socket(context, ZMQ_REP);
	zmq_bind(frontend, "tcp://*:5559");
	if (!init_recver(frontend)) return;
	printf("filename= %s\nfilesize=%d\nsplitsize=%d\nthreads=%d\n",file.filename,file.filesize,file.spliteSize,file.threadNum);
	zmq_close(frontend);

	//frontend = zmq_socket(context, ZMQ_ROUTER);
	void *backend = zmq_socket(context, ZMQ_DEALER);
	//zmq_bind(frontend, "tcp://*:5559");
	zmq_bind(backend, "inproc://workers");
	//  Initialize poll set


	pthread_t *pt;
	pt = (pthread_t *)malloc(file.threadNum * sizeof(pthread_t));
	int *ptid;
	ptid = (int *)malloc(file.threadNum * sizeof(int));
	//获取当前时间
	clock_t start;
	start = clock();
	double time_start = (double)start / CLOCKS_PER_SEC;
	markTime = time_start;
	//多线程接收
	for (int mthread = 0; mthread < file.threadNum; mthread++) {
		ptid[mthread] = pthread_create(&pt[mthread], NULL, recver, context);
	}
	pthread_exit(NULL);
	/*pthread_t pRecv;
	int ret2 = pthread_create(&pRecv, NULL, rrworker, context);
	if (ret2 != 0)
	{
		printf("pthread_create error: error_code=%d", ret2);
	}*/
	//  Switch messages between sockets
	zmq_pollitem_t items[] = {
	{ frontend, 0, ZMQ_POLLIN, 0 },
	{ backend,  0, ZMQ_POLLIN, 0 }
	};
	while (1) {
		zmq_msg_t message;
		zmq_poll(items, 2, -1);
		if (items[0].revents & ZMQ_POLLIN) {
				zmq_msg_init(&message);
				zmq_msg_recv(&message, frontend, 0);
				int more = zmq_msg_more(&message);
				zmq_msg_send(&message, backend, more ? ZMQ_SNDMORE : 0);
		}
		if (items[1].revents & ZMQ_POLLIN) {
				zmq_msg_init(&message);
				zmq_msg_recv(&message, backend, 0);
				int more = zmq_msg_more(&message);
				zmq_msg_send(&message, frontend, more ? ZMQ_SNDMORE : 0);
				zmq_msg_close(&message);
		}
	}
	//  We never get here, but clean up anyhow
	//pthread_exit(NULL);
	printf("s thread end");
	fclose(rF);
	zmq_close(frontend);
	zmq_close(backend);
	zmq_ctx_destroy(context);
	pthread_mutex_destroy(&sended_lock);
	pthread_mutex_destroy(&printf_lock);
	for (int i = 0; i < 3; i++) {
		pthread_mutex_destroy(&recvsize[i]);
	}
	return 0;
}