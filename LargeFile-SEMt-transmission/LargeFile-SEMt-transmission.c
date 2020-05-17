// LargeFile-SEMt-transmission.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//
#include <zmq.h>
#include "zhelpers.h"
#include <pthread.h>
//#include "server.h"
//#include "client.h"
//#include"fConfig.h"
#pragma comment(lib,"pthreadVC2.lib")
struct mypara
{
	void *context;
	int a;
};
void msg_send(char *buffer, zmq_msg_t *mf,void *sender) {
	zmq_msg_init_size(&mf, sizeof(buffer));
	memcpy(zmq_msg_data(&mf), buffer, sizeof(buffer));
	zmq_msg_send(&mf, sender, 0);
	zmq_msg_close(&mf);
}
void msg_recv(char *buffer, void *recver) {
	zmq_msg_t message;
	zmq_msg_init(&message);
	zmq_msg_recv(&message, recver, 0);
	int size = zmq_msg_size(&message);
	memset(buffer, 0, sizeof(buffer));
	memcpy(buffer, zmq_msg_data(&message), size);
	zmq_msg_close(&message);
	buffer[size] = 0;
}
void tpsender(struct mypara *my) {

	void *context = my->context;
	void *requester = zmq_socket(context, ZMQ_REP);
	zmq_connect(requester, "inproc://senders");
	int request_nbr = my->a;
	s_send(requester, "Hello");
	char *string = s_recv(requester);
	//char *string;
	//msg_recv(string, requester);
	char c[15];
	int id = GetCurrentThreadId();
	itoa(id, c, 10);
	void*cs;
	EnterCriticalSection(&cs);
	printf("%s", c);
	printf("收到应答 %d [%s]\n", request_nbr,string);
	LeaveCriticalSection(&cs);
	free(string);
}
void testsender() {
	Sleep(2000);
	void *context = zmq_init(1);
	//  用于和senders进行通信的套接字
	//void *senders = zmq_socket(context, ZMQ_DEALER);
	//zmq_bind(senders, "inproc://senders");
	//  用于和服务端通信的套接字
	void *requester = zmq_socket(context, ZMQ_REQ);
	zmq_connect(requester, "tcp://localhost:5555");
	struct mypara my[10];
	//启用senders池
	int request_nbr;
	for (request_nbr = 0; request_nbr != 10; request_nbr++) {
		//my[request_nbr].context = context;
		//my[request_nbr].a = request_nbr;
		//pthread_t sender;
		//pthread_create(&sender, NULL, tpsender, &my[request_nbr]);
		//tpsender(&my[request_nbr]);
		//continue;
		s_send(requester, "Hello");
		char *string = s_recv(requester);
		printf("收到应答 %d [%s]\n", request_nbr, string);
		free(string);
	}
	//  启动队列装置
	//zmq_device(ZMQ_QUEUE, requester, senders);
	zmq_close(requester);
	zmq_term(context);
	return 0;
}

void testrecver() {
	Sleep(2000);

	void *context = zmq_init(1);

	//  用于何客户端通信的套接字
	void *responder = zmq_socket(context, ZMQ_REP);
	zmq_connect(responder, "tcp://localhost:5560");

	while (1) {
        //  等待下一个请求
        char *string = s_recv (responder);
        printf ("Received request: [%s]\n", string);
        free (string);
 
        //  做一些“工作”
        Sleep (1);
 
        //  返回应答信息
        s_send (responder, "World");
	}
	//  程序不会运行到这里，不过还是做好清理工作
	zmq_close(responder);
	zmq_term(context);
	return 0;
}
static void *
worker_routine(void *context) {
	//  连接至代理的套接字
	void *receiver = zmq_socket(context, ZMQ_REP);
	zmq_connect(receiver, "inproc://workers");
	printf("ready\n");
	//pthread_t pid= pthread_self();
	while (1) {
		printf("ready1\n");
		char *string = s_recv(receiver);
		printf("Received request: [%s]\n", string);
		free(string);
		//  工作
		Sleep(1000);
		//  返回应答
		char c[15];
		int id = GetCurrentThreadId();
		itoa(id, c, 10);
		char *reqs = (char *)malloc(strlen(c) + strlen("World"));
		sprintf(reqs, "%s%s", c, "World");
		s_send(receiver, reqs);
	}
	zmq_close(receiver);
	return NULL;
}
void testrouter() {
	void *context = zmq_init(1);

	//  用于和client进行通信的套接字
	void *clients = zmq_socket(context, ZMQ_ROUTER);
	zmq_bind(clients, "tcp://*:5555");

	//  用于和worker进行通信的套接字
	void *workers = zmq_socket(context, ZMQ_DEALER);
	zmq_bind(workers, "inproc://workers");

	//  启动一个worker池
	int thread_nbr;
	for (thread_nbr = 0; thread_nbr < 5; thread_nbr++) {
		pthread_t worker;
		pthread_create(&worker, NULL, worker_routine, context);
	}
	//  启动队列装置
	zmq_device(ZMQ_QUEUE, clients, workers);

	//  程序不会运行到这里，但仍进行清理工作
	zmq_close(clients);
	zmq_close(workers);
	zmq_term(context);
	return 0;
}
int main() {
	//testrouter();
	pthread_t pSend, pRecv,pRouter;

	//int ret = pthread_create(&pSend, NULL, zsend, NULL);
	int ret = pthread_create(&pSend, NULL, testsender, NULL);
	if (ret != 0)
	{
		printf("pthread_create error: error_code=%d", ret);
	}
	////int ret2 = pthread_create(&pRecv, NULL, zrecv, NULL);
	//int ret2 = pthread_create(&pRecv, NULL, testrecver, NULL);
	//if (ret2 != 0)
	//{
	//	printf("pthread_create error: error_code=%d", ret2);
	//}
	int ret3 = pthread_create(&pRouter, NULL, testrouter, NULL);
	if (ret3 != 0)
	{
		printf("pthread_create error: error_code=%d", ret3);
	}
	pthread_exit(NULL);
}
