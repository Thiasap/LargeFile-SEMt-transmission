#pragma pack(1)
#include "serverRecv.h"
FILE *sF;
struct sFile file;
pthread_mutex_t printf_lock;
pthread_mutex_t sended_lock;	//发送块标记锁
char *sended;					//发送块标记
double time_start, markTime;	//开始时间，上次获得的时间
pthread_mutex_t recvsize[3];	//已接收数据大小变量锁
int recv_kmg[3];				//已接收数据大小
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
	//初始化锁
	pthread_mutex_init(&sended_lock, NULL);
	pthread_mutex_init(&printf_lock, NULL);
	for (int i = 0; i < 3; i++) {
		recv_kmg[i] = 0;
		pthread_mutex_init(&recvsize[i], NULL);
	}
	return 1;
}
void rrworker(void *context) {
	//void *context = zmq_ctx_new();

	//  Socket to talk to clients
	void *responder = zmq_socket(context, ZMQ_REP);
	zmq_connect(responder, "inproc://workers");

	while (1) {
		//  Wait for next request from client
		char *string = s_recv(responder);
		printf("Received request: [%s]\n", string);
		free(string);

		//  Do some 'work'
		Sleep(1000);

		//  Send reply back to client
		s_send(responder, "World");
	}
	//  We never get here, but clean up anyhow
	zmq_close(responder);
	zmq_ctx_destroy(context);
	return 0;

}
void rrbroker() {
	void *context = zmq_ctx_new();
	void *frontend = zmq_socket(context, ZMQ_REP);
	zmq_bind(frontend, "tcp://*:5559");
	if (!init_recver(frontend)) return;
	frontend = zmq_socket(context, ZMQ_ROUTER);
	void *backend = zmq_socket(context, ZMQ_DEALER);
	zmq_bind(frontend, "tcp://*:5559");
	zmq_bind(backend, "inproc://workers");
	//  Initialize poll set
	zmq_pollitem_t items[] = {
		{ frontend, 0, ZMQ_POLLIN, 0 },
		{ backend,  0, ZMQ_POLLIN, 0 }
	};

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
		ptid[mthread] = pthread_create(&pt[mthread], NULL, rrworker, context);
	}
	/*pthread_t pRecv;
	int ret2 = pthread_create(&pRecv, NULL, rrworker, context);
	if (ret2 != 0)
	{
		printf("pthread_create error: error_code=%d", ret2);
	}*/
	//  Switch messages between sockets
	while (1) {
		zmq_msg_t message;
		zmq_poll(items, 2, -1);
		if (items[0].revents & ZMQ_POLLIN) {
			while (1) {
				//  Process all parts of the message
				zmq_msg_init(&message);
				zmq_msg_recv(&message, frontend, 0);
				int more = zmq_msg_more(&message);
				zmq_msg_send(&message, backend, more ? ZMQ_SNDMORE : 0);
				zmq_msg_close(&message);
				if (!more)
					break;      //  Last message part
			}
		}
		if (items[1].revents & ZMQ_POLLIN) {
			while (1) {
				//  Process all parts of the message
				zmq_msg_init(&message);
				zmq_msg_recv(&message, backend, 0);
				int more = zmq_msg_more(&message);
				zmq_msg_send(&message, frontend, more ? ZMQ_SNDMORE : 0);
				zmq_msg_close(&message);
				if (!more)
					break;      //  Last message part
			}
		}
	}
	//  We never get here, but clean up anyhow
	zmq_close(frontend);
	zmq_close(backend);
	zmq_ctx_destroy(context);
	pthread_exit(NULL);
	pthread_mutex_destroy(&sended_lock);
	pthread_mutex_destroy(&printf_lock);
	for (int i = 0; i < 3; i++) {
		pthread_mutex_destroy(&recvsize[i]);
	}
	return 0;
}