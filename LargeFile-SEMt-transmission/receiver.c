#pragma pack(1)
#include <zmq.h>
#include <stdio.h>
#include"zhelpers.h"
#include"server.h"
#include"fConfig.h"
FILE *rF;
void *context;
struct sFile file;
//  用于发送开始信号的套接字
void *sink;
const char * pAddr;
int sizezzz;
void z_recv(FILE *recvF) {
	sizezzz = 0;
	char buffer[sizeof(filebuf)] = { 0 };
	filebuf fbuf;
	int a = 0;
	while (1) {
		zmq_recv(sink, buffer, sizeof(filebuf), 0);
		s_send(sink, "");
		memcpy((void*)&fbuf, buffer, sizeof(filebuf));
		int size = fbuf.size;
		sizezzz += size;
		fseek(recvF, fbuf.index, SEEK_SET);
		//printf("\t\t\t\t\t\t\tss %d %d %d %d pos ftell %d pos %d\n", fbuf.buff[0], fbuf.buff[1], fbuf.buff[2], fbuf.buff[3],ftell(recvF), fbuf.index);
		fwrite(fbuf.buff, sizeof(char), size, recvF);
		fflush(recvF);
		//printf("sss%d",fbuf.splitEnd);
		if (fbuf.splitEnd == 1) {
			printf("\n[server]recv...%d\n",sizezzz);
			s_send(sink, "w");//接收完了
		}
		//printf("[server]mark=%d...time = %d\n",fbuf.mark,time);
		if (fbuf.mark == 1) {
			char *p = s_recv(sink);
			if (strcmp(p, "wait") == 0) s_send(sink, "all recv");
			printf("[server]all recieved!\n");
			break;
		}

	}
	fclose(recvF);
	zmq_close(sink);
	zmq_ctx_destroy(context);
	printf("[server]recv end\n");
	return 0;
}
void zrecv() {
	context = zmq_ctx_new();
	//  用于发送开始信号的套接字
	sink = zmq_socket(context, ZMQ_REP);
	pAddr = "tcp://*:5558";
	zmq_bind(sink, pAddr); 
	char *pf;
	pf = (char *)malloc(sizeof(file));
	zmq_recv(sink, pf, sizeof(file), 0);
	memcpy((char *)&file, pf, sizeof(file));
	if (file.mark != 111) {
		printf("[server]pf mark: \n");
		for (int i = 0; i < 10; i++) {
			printf("%d ", pf[i]);
		}
		printf("\n[server]init Error! Cannot recv file info");
		return 0;
	}
	printf("[server]recv file name: %s\n", file.filename);
	//reply
	s_send(sink, file.filename);
	char *tmp = (char*)malloc(sizeof(file.filename));
	memcpy(tmp, file.filename, sizeof(file.filename));
	memcpy(file.filename, "Recv_",6);
	strcat(file.filename, tmp);
	free(tmp);
	//strcat(file.filename, "12");
	if ((rF = fopen(file.filename, "wb")) == NULL) {
		printf("[server]stop because open file err\n");
		return 0;
	}
#if 1
	remove(rF);
#endif
	//s_send(sink, "ready");
	z_recv(rF);
	//

}