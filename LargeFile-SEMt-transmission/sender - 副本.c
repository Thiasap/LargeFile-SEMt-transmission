#include <zmq.h>
#include <stdio.h>
#include"client.h"
#include"fConfig.h"
#define ARGV "test.txt"
int send_msg(const char* msg, void *pSock)//发送消息
{
	if (zmq_send(pSock, msg, sizeof(msg), 0) < 0){
		fprintf(stderr, "send message faild\n");
		return 0;
	}
	return 1;
}
char * recive_msg(void *pSock)//接收消息
{
	char szMsg[1024] = { 0 };
	errno = 0;
	if (zmq_recv(pSock, szMsg, sizeof(szMsg), 0) < 0){
		printf("error = %s\n", zmq_strerror(errno));
		return -1;
	}
	return szMsg;
}
struct file_route_msg
{
	unsigned int path_len;
	char path[255];
};

int read_file_frame(char* filename, int pos, char* buffer, int length) {
	FILE* fp = fopen(filename, "rb");
	if (NULL == fp) {
		printf("[client]fopen failed!\n");
		return -1;
	}
	fseek(fp, pos, SEEK_SET);
	int size = fread(buffer, 1, length, fp);
	if (size != length) {
		printf("[client]read over!\n");
	}
	fclose(fp);
	return size;
}
void zsend(void *se) {
	void *context = zmq_init(1);
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
	while (1) {
		memset(buffer, 0, sizeof buffer);
		int size;
		size = read_file_frame("twly.png", pos, buffer,(s_e->end == -1||FILE_FRAME_SIZE < ssindex)? FILE_FRAME_SIZE:ssindex);
		pos += size;
		ssindex -= size;
		if (size == FILE_FRAME_SIZE) {
			zmq_msg_t msg_frame;
			zmq_msg_init_size(&msg_frame, size);
			memcpy(zmq_msg_data(&msg_frame), buffer, size);
			zmq_msg_send(&msg_frame, sender, ZMQ_SNDMORE);
			zmq_msg_close(&msg_frame);
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
			printf("[client]read file content over %d bytes\n", pos);
			zmq_msg_t msg_frame;
			zmq_msg_init_size(&msg_frame, size);
			memcpy(zmq_msg_data(&msg_frame), buffer, size);
			zmq_msg_send(&msg_frame, sender, 0);
			zmq_msg_close(&msg_frame);
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
	zmq_ctx_destroy(context);
	printf("[client]send end\n");
	return 0;
}