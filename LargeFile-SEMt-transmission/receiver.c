#include"server.h"
#include"Cryptions.h"
FILE *rF;
void *context;
struct sFile file;
//  用于发送开始信号的套接字
void *sink;
const char * pAddr;
int sizezzz;
char pwd[65];	//密码
void z_recv(FILE *recvF) {
	sizezzz = 0;
	char buffer[sizeof(filebuf)] = { 0 };
	filebuf fbuf;
	int a = 0;
	while (1) {
		zmq_recv(sink, buffer, sizeof(filebuf), 0);
		memcpy((void*)&fbuf, buffer, sizeof(filebuf));
		int size = fbuf.size;
		s_send(sink, "");
		if (file.useCrypt !=0) {
			crypt_buffer cb;
			memcpy(cb.buff, fbuf.buff, size);
			cb.size = size;
			/*printf("before decrypt:\n");
			phex(cb.buff);*/
			Decrypt(&cb, pwd, file.useCrypt);
			/*printf("after decrypt: %s\n", cb.buff);
			printf("[ss]%s",cb.buff);*/
			size = cb.size;
			memcpy(fbuf.buff, cb.buff, size);
		}
		sizezzz += size;
		fseek(recvF, fbuf.index, SEEK_SET);
		//printf("\t\t\t\t\t\t\tss %d %d %d %d pos ftell %d pos %d\n", fbuf.buff[0], fbuf.buff[1], fbuf.buff[2], fbuf.buff[3],ftell(recvF), fbuf.index);

		fwrite(fbuf.buff, sizeof(char), size, recvF);
		//fflush(recvF);
		//printf("sss%d",fbuf.splitEnd);
		if (fbuf.splitEnd == 1) {
			printf("\n[server]recv...%d\n",sizezzz);
			s_send(sink, "w");//接收完了
		}
		//printf("[server]mark=%d...time = %d\n",fbuf.mark,time);
		if (fbuf.mark == 1) {
			char *p = s_recv(sink);
			if (strcmp(p, "wait") == 0) s_send(sink, "all recv");
			printf("[server]all received!\n");
			break;
		}

	}
	fclose(recvF);
	printf("[server]recv end\n");
	return 0;
}

void ready_recv(argv_info * info) {

	char protocol[] = "tcp://";
	int pAddrSize = strlen(protocol) + strlen(info->ip) + strlen(info->port) + 2;
	char *pAddr = (char *)malloc(pAddrSize);
	memset(pAddr, 0, pAddrSize);
	memcpy(pAddr, protocol, strlen(protocol) + 1);
	strcat(pAddr, info->ip);
	pAddr[strlen(pAddr)] = ':';
	pAddr[strlen(pAddr) + 1] = 0;
	strcat(pAddr, info->port);
	//pAddr = "tcp://*:5558";
	zmq_bind(sink, pAddr); 
	printf("[server]Listening at port %s...\n", info->port);
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
	printf("[server]file size: %d\n", file.filesize);
	//reply
	s_send(sink, file.filename);
	char *tmp = (char*)malloc(sizeof(file.filename));
	memcpy(tmp, file.filename, sizeof(file.filename));
	memcpy(file.filename, "Recv_",6);
	strcat(file.filename, tmp);
	free(tmp);
	if (file.useCrypt != 0) {
		//char * pwdtmp;
		//pwdtmp = (char *)malloc(strlen(info->passwd) + 1);
		memcpy(pwd, StrSHA256(info->passwd, (strlen(info->passwd))),65);
		//memcpy(pwd, info->passwd, strlen(info->passwd) + 1);
	}
	//strcat(file.filename, "12");
	if ((rF = fopen(file.filename, "wb")) == NULL) {
		printf("[server]stop because open file err\n");
		return 0;
	}
	if (setvbuf(rF, NULL, _IOLBF, file.spliteSize) != 0)
	{
		printf("failed to set up buffer for output file\n");
	}
#if 1
	remove(rF);
#endif
	//s_send(sink, "ready");
	z_recv(rF);
	//

}
void zrecv(argv_info * info) {
	if ((context = zmq_ctx_new()) == NULL)
		return 0;
	//  用于发送开始信号的套接字
	if ((sink = zmq_socket(context, ZMQ_REP)) == NULL) {
		zmq_ctx_destroy(context);
		return 0;
	}
	while (1) {
		ready_recv(info);
	}
	zmq_close(sink);
	zmq_ctx_destroy(context);
}