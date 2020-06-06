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
LARGE_INTEGER start_time, mark_time, CPU_fre;
int recv_kmg[3];//0,kB,1,MB,2,GB
//更新已接收的数据大小，大约200ms更新一次
void updateTime(int size, int end) {
	recv_kmg[0] += size / 1024;
	if (recv_kmg[0] >= 1024) {
		recv_kmg[1]++;
		recv_kmg[0] = recv_kmg[0] - 1024;
	}
	if (recv_kmg[1] >= 1024) {
		recv_kmg[2]++;
		recv_kmg[1] = recv_kmg[1] - 1024;
	}
	LARGE_INTEGER now_time;
	QueryPerformanceCounter(&now_time);
	double time_pass = ((double)now_time.QuadPart - (double)mark_time.QuadPart) / (double)CPU_fre.QuadPart * 1000;
	if (end == 1 || time_pass >= 200) {
		printf("[server]%dG %dM %dK received\n", recv_kmg[2], recv_kmg[1], recv_kmg[0]);
		mark_time = now_time;
	}
}
//主要接收数据的函数
void z_recv(FILE *recvF) {
	sizezzz = 0;
	int a = 0;
	while (1) {
		char *buffer = malloc(sizeof(filebuf));
		memset(buffer, 0, sizeof(filebuf));
		filebuf *fbuf;
		fbuf = (filebuf *)malloc(sizeof(filebuf));
		zmq_recv(sink, buffer, sizeof(filebuf), 0);
		memcpy(fbuf, buffer, sizeof(filebuf));
		int size = fbuf->size;
		s_send(sink, "");
		if (file.useCrypt !=0) {
			crypt_buffer *cb;
			cb = (crypt_buffer*)malloc(sizeof(crypt_buffer));
			memcpy(cb->buff, fbuf->buff, size);
			cb->size = size;
			/*printf("before decrypt:\n");
			phex(cb.buff);*/
			Decrypt(cb, pwd, file.useCrypt);
			/*printf("after decrypt: %s\n", cb.buff);
			printf("[ss]%s",cb.buff);*/
			size = cb->size;
			memcpy(fbuf->buff, cb->buff, size);
			free(cb);
			cb = NULL;
		}
		fseek(recvF, fbuf->index, SEEK_SET);
		//printf("\t\t\t\t\t\t\tss %d %d %d %d pos ftell %d pos %d\n", fbuf.buff[0], fbuf.buff[1], fbuf.buff[2], fbuf.buff[3],ftell(recvF), fbuf.index);

		fwrite(fbuf->buff, sizeof(char), size, recvF);
		updateTime(size, fbuf->mark);
		//fflush(recvF);
		//printf("sss%d",fbuf.splitEnd);
		if (fbuf->splitEnd == 1) {
			//printf("[server]recv...%d\n",sizezzz);
			s_send(sink, "w");//接收完了
		}
		//printf("[server]mark=%d...time = %d\n",fbuf.mark,time);
		if (fbuf->mark == 1) {
			char *p = s_recv(sink);
			if (strcmp(p, "wait") == 0) s_send(sink, "all recv");
			printf("[server]all received!\n");
			free(buffer);
			free(fbuf);
			break;
		}
		free(fbuf);
		free(buffer);
	}
	printf("[server]recv end\n");
	fclose(recvF);
	char SHA256[65];
	FileSHA256(file.filename, SHA256);
	printf("File SHA256 From sender:\n%s\nFile SHA256 received:\n%s\n", file.sha256, SHA256);
	return 0;
}
//初始化，监听端口，接收第一个信息包
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
		memcpy(pwd, StrSHA256(info->passwd, (strlen(info->passwd))),65);
	}
	if ((rF = fopen(file.filename, "wb")) == NULL) {
		printf("[server]stop because open file err\n");
		return 0;
	}
	if (setvbuf(rF, NULL, _IOLBF, file.spliteSize) != 0){
		printf("failed to set up buffer for output file\n");
	}
#if 0
	remove(rF);
#endif
	//发送的数据信息大小初始化
	for (int i = 0; i < 3; i++) {
		recv_kmg[i] = 0;
	}
	//获取当前时间
	QueryPerformanceFrequency(&CPU_fre);
	QueryPerformanceCounter(&start_time);
	mark_time = start_time;
	z_recv(rF);
	//

}
//总函数，创建套接字，并在初始化后循环监听端口，方便第二次传输数据
void zrecv(argv_info * info) {
	if ((context = zmq_ctx_new()) == NULL)
		return 0;
	//  用于发送开始信号的套接字
	if ((sink = zmq_socket(context, ZMQ_REP)) == NULL) {
		zmq_ctx_destroy(context);
		return 0;
	}
	int ipv6 = 1;
	zmq_setsockopt(sink, ZMQ_IPV6, &ipv6, 4);
	while (1) {
		ready_recv(info);
	}
	zmq_close(sink);
	zmq_ctx_destroy(context);
}