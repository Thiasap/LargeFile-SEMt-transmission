// LargeFile-SEMt-transmission.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//
#include <zmq.h>
#include <pthread.h>
#include "zhelpers.h"
#include "server.h"
#include<czmq.h>
#include "client.h"
#include"fConfig.h"

//#include"clientSend.h"
//#include "serverRecv.h"
argv_info param;
char lastparam[3];
int Str2Int(char * buff){
	int num = 0;
	int index = 0;
	for (; buff[index] >= '0' && buff[index] <= '9'; index++){
		num = num * 10 + buff[index] - '0';
	}
	return num;
}
void split_ip_port(char *p) {
	char *tmp;
	for (int i = 0; i < strlen(p); i++) {
		if (p[i] == ':') {
			tmp = (char*)malloc(sizeof(char)*i + 1);
			memcpy(tmp, p, i);
			char lp[] = "-l";
			memcpy(lastparam, lp, 3);
			dealwith_param(tmp); 
			int portsize = (strlen(p) - i);
			tmp = (char*)malloc(sizeof(char)*portsize);
			p = &p[i+1];
			memcpy(tmp, p, portsize);
			lp[1] ='p';
			memcpy(lastparam, lp, 3);
			dealwith_param(tmp);
			memcpy(tmp, p, i);
			break;
		}
	}
}
void help() {
	printf("Just support Windows\n");
	printf("usage:\n");
	printf("\trun as receiver: .\semt.exe -r [[-l listen_ip] -p port] or [-lp listen_ip:port] -c crypt_mode -k passwd -f filename\n");
	printf("\trun as sender: .\semt.exe -s [[-l connect_ip] -p port] or [-lp connect_ip:port] -c crypt_mode -k passwd -f filename\n");
	printf("\t\t-r run as receiver and receive file,if you don't set filename, it will be set add \"Recv_\" before default name.\n");
	printf("\t\t-r run as sender and send file.\n");
	printf("\t\t\tconnect_ip is your friend's ip and port is his port\n");
	printf("\t\t\n");
	printf("PLEASE negotiate the password by OTHER SAFE means.\n");
	printf("You need set AT LEAST ip, port and file.\n");
	printf("If you are receiver, you cat don't set ip, and it will listen all ip(*:port).\n");
	printf("If you use cryption, you must set password, and send the password to others.\n");
	//r /-r -lp l/3  /-r -lp -f file/5 /-r -l l -p p/5
}
//return 0:error, 1: -x, 2: get parameter end;
int dealwith_param(char *p) {
	int flag = 0;
	if (strcmp("-r", p) == 0) { 
		param.work_mode = 'r';
		return 2;
	}else if (strcmp("-s", p) == 0) {
		param.work_mode = 's';
		return 2;
	}else if (strcmp("-l", p) == 0 || strcmp("-p", p) == 0 
		|| strcmp("-lp", p) == 0 || strcmp("-c", p) == 0 
		|| strcmp("-k", p) == 0 || strcmp("-f", p) == 0) {
		memcpy(lastparam, p, 3);
		return 1;
	}
	if (strcmp("-l", lastparam) == 0) {
		if(p[0] == '*') memcpy(param.ip, p, 1);
		else if ((!(strlen(p) >= 7 && strlen(p) <= 15))) {
			printf("Input IP error!\n");
			return 0;
		}else {
			memcpy(param.ip, p, strlen(p)+1);
		}
	}else if (strcmp("-p", lastparam) == 0) {
		if (!(strlen(p) < 6)) {
			printf("Input PORT error!\n");
			return 0;
		}else {
			int port = Str2Int(p);
			if (port >= 65535) {
				printf("Input PORT error!\n");
				return 0;
			}else if(port<=1000) printf("You are using prot in 0-1000, It may be dangerous!\n");
			memcpy(param.port, p, strlen(p)+1);
		}
	}else if (strcmp("-lp", lastparam) == 0) {
		split_ip_port(p);
	}else if (strcmp("-c", lastparam) == 0) {
		param.crypt_mode = p[0] - '0';
	}else if (strcmp("-k", lastparam) == 0) {
		if (strlen(p) > 32) {
			printf("password too long!\n");
			return 0;
		}
		memcpy(param.passwd, p, strlen(p)+1);
	}else if (strcmp("-f", lastparam) == 0) {
		if (strlen(p) > 254) {
			printf("filename too long!\n");
			return 0;
		}
		memcpy(param.filename, p, strlen(p)+1);
	}
}
int check_param() {
	if (param.work_mode != 'r' && param.work_mode != 's') return 0;
	if (strlen(param.port) == 0) return 0;
	if (strlen(param.ip) == 0) {
		if (param.work_mode == 'r') {
			param.ip[0] = '*';
			param.ip[1] = 0;
		}else return 0;
	}
	if (param.crypt_mode > 0 && param.crypt_mode<=CRYPT_MODES) {
		if (strlen(param.passwd) == 0)return 0;
	}
	if (param.work_mode == 's'&&(
		strlen(param.filename) == 0 || param.ip[0]=='*'
		)) return 0;
	return 1;
}
int main(int argc, char *argv[]) {
	//printf("++\n");
	if (argc < 3) {
		printf("maybe you are input error\n");
		help();
		return 0;
	}
	for (int i = 1; i < argc; i++) {
		if (dealwith_param(argv[i]) == 0) {
			help();
			return 0;
		}
		//printf("++%s\n", argv[i]);
	}
	if (check_param() == 0) {
		help();
		return 0;
	}
	printf("mode: %c  ip: %s  port: %s  crypt: %d ", param.work_mode, param.ip, param.port, param.crypt_mode);
	if(param.passwd[0]!=0) printf("passwd: %s ", param.passwd);
	if (param.filename[0] != 0) printf("filename: %s  \n", param.filename);
	//char work_mode;		//-r
	//char ip[16];		//-l
	//char port[6];		//-p
	//int crypt_mode;		//-c
	//char passwd[33];	//-k
	//char filename[255];	//-f
	return 0;
	pthread_t pRouter;
	int ret3 = pthread_create(&pRouter, NULL, zrecv, NULL);
	if (ret3 != 0)
	{
		printf("pthread_create error: error_code=%d", ret3);
	}
	allsend();
	pthread_join(pRouter,NULL);
}
