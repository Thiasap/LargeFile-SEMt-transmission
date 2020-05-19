#include <zmq.h>
#include <stdio.h>
#include"server.h"
#include"fConfig.h"

void zrecv() {
	void *context = zmq_init(1);

	//  用于发送开始信号的套接字
	void *sink = zmq_socket(context, ZMQ_PULL);
	const char * pAddr = "tcp://*:5558";
	zmq_bind(sink, pAddr);
	FILE *recvF;
	recvF = fopen("recv.png", "wb");
	if (recvF == NULL)
	{
		printf("[server]stop because open file err\n");
		return 0;
	}
	remove(recvF);
	zmq_msg_t message;
	char buffer[FILE_FRAME_SIZE] = { 0 };
	while (1) {
		printf("[server]recv...\n");
		zmq_msg_init(&message);
		zmq_msg_recv(&message, sink, 0);
		int size = zmq_msg_size(&message);
		printf("[server]recv size %d\n", size);
		memset(buffer, 0, sizeof(buffer));
		memcpy(buffer, zmq_msg_data(&message), size);
		fwrite(buffer, sizeof(char), size, recvF);
		zmq_msg_close(&message);
		int64_t more;
		size_t more_size = sizeof(more);
		zmq_getsockopt(sink, ZMQ_RCVMORE, &more, &more_size);
		if (!more) {
			zmq_msg_close(&message);
			break;
		}

	}
	fclose(recvF);
	zmq_close(sink);
	zmq_ctx_destroy(context);
	printf("[server]recv end\n");
	return 0;
}