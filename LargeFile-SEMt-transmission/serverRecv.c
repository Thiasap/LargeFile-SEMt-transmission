#pragma pack(1)
#include "serverRecv.h"
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
	void *frontend = zmq_socket(context, ZMQ_ROUTER);
	void *backend = zmq_socket(context, ZMQ_DEALER);
	zmq_bind(frontend, "tcp://*:5559");
	zmq_bind(backend, "inproc://workers");

	//  Initialize poll set
	zmq_pollitem_t items[] = {
		{ frontend, 0, ZMQ_POLLIN, 0 },
		{ backend,  0, ZMQ_POLLIN, 0 }
	};
	pthread_t pRecv;
	int ret2 = pthread_create(&pRecv, NULL, rrworker, context);
	if (ret2 != 0)
	{
		printf("pthread_create error: error_code=%d", ret2);
	}
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
	return 0;
}