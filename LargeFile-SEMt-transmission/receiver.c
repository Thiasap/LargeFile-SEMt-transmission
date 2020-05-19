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

void z_recv(FILE *recvF) {
	char buffer[sizeof(filebuf)] = { 0 };
	filebuf fbuf;
	int time = 0;
	while (1) {
		printf("[server]recv...\n");
		zmq_recv(sink, buffer, sizeof(filebuf), 0);
		memcpy((void*)&fbuf, buffer, sizeof(filebuf));
		int size = fbuf.size;
		fseek(recvF, fbuf.index, SEEK_SET);
		fwrite(fbuf.buff, sizeof(char), size, recvF);
		time++;
		//printf("[server]mark=%d...time = %d\n",fbuf.mark,time);
		if (fbuf.mark == 1) break;

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
	sink = zmq_socket(context, ZMQ_PULL);
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
	//zmq_send(sink, file.filename,sizeof(file.filename),0);
	strcat(file.filename, "_recv");
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