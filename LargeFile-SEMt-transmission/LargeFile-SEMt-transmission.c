#include <zmq.h>
#include <pthread.h>
#include "zhelpers.h"
#include "server.h"
#include "client.h"
#include"fConfig.h"
#include"sha256.h"
#include "aes.h"
#include"aes.h"
argv_info param;
char lastparam[3];//上一个参数-l/-p/-t等，用于决定下一个参数的类型
//输入的参数都是字符串，
//分片大小和线程数需要转成Int型
int Str2Int(char * buff) {
	int num = 0;
	int index = 0;
	for (; buff[index] >= '0' && buff[index] <= '9'; index++) {
		num = num * 10 + buff[index] - '0';
	}
	return num;
}
//将类似*:7997格式的IP和端口分开
void split_ip_port(char *p) {
	char *tmp;
	for (int i = 0; i < strlen(p); i++) {
		if (p[i] == ':') {
			tmp = (char*)malloc(sizeof(char)*i + 1);
			memset(tmp, 0, i + 1);
			memcpy(tmp, p, i);
			char lp[] = "-l";
			memcpy(lastparam, lp, 3);
			dealwith_param(tmp);
			int portsize = (strlen(p) - i);
			tmp = (char*)malloc(sizeof(char)*portsize);
			memset(tmp, 0, i + 1);
			p = &p[i + 1];
			memcpy(tmp, p, portsize);
			lp[1] = 'p';
			memcpy(lastparam, lp, 3);
			dealwith_param(tmp);
			memcpy(tmp, p, i);
			break;
		}
	}
}
//输出帮助信息
void help() {
	printf("Just support Windows\n");
	printf("usage:\n");
	printf("\trun as receiver: .\\semt.exe -r [[-l listen_ip] -p port] or [-lp listen_ip:port] -c crypt_mode -k passwd -f filename\n");
	printf("\trun as sender: .\\semt.exe -s [[-l connect_ip] -p port] or [-lp connect_ip:port] -c crypt_mode -k passwd -f filename -t Threads\n");
	printf("\t\t -z SplitSize   BYTE. For sender. Splite file will load to RAM. If your ram is too small, change it to smaller\n");
	printf("\t\t -t Threads     For sender. Use more threads to send file, don't be too big. Max is 32.\n");
	printf("\t\t-r run as receiver and receive file,if you don't set filename, it will be set add \"Recv_\" before default name.\n");
	printf("\t\t-s run as sender and send file.\n");
	printf("\t\t\tconnect_ip is your friend's ip and port is his port\n");
	printf("\t\t1-3, AES-(128/192/256)-cbc, 4,DES, 5,3DES\n");
	printf("PLEASE negotiate the password by OTHER SAFE means.\n");
	printf("You need set AT LEAST ip, port and file.\n");
	printf("If you are receiver, you cat don't set ip, and it will listen all ip(*:port).\n");
	printf("If you use cryption, you must set password, and send the password to others.\n");
	//r /-r -lp l/3  /-r -lp -f file/5 /-r -l l -p p/5
}
//获取ip port等参数并存入argv_info
int dealwith_param(char *p) {
	if (strcmp("-r", p) == 0) { 
		param.work_mode = 'r';
		return 2;
	}else if (strcmp("-s", p) == 0) {
		param.work_mode = 's';
		return 2;
	}else if (strcmp("-l", p) == 0 || strcmp("-p", p) == 0
		|| strcmp("-lp", p) == 0 || strcmp("-c", p) == 0
		|| strcmp("-t", p) == 0 || strcmp("-z", p) == 0 
		|| strcmp("-k", p) == 0 || strcmp("-f", p) == 0) {
		memcpy(lastparam, p, 3);
		return 1;
	}
	if (strcmp("-l", lastparam) == 0) {
		if(p[0] == '*') memcpy(param.ip, p, 1);
		else if ((!(strlen(p) >= 2 && strlen(p) <= 42))) {
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
		if (strlen(p) > 256) {
			printf("file path/name too long!\n");
			return 0;
		}
		int pathsize;
		memset(param.filepath, 0, 128);
		for (pathsize = strlen(p); pathsize >= 0; pathsize--) {
			if (p[pathsize] == '\\' || p[pathsize] == '/') {
				memcpy(param.filepath, p, pathsize+1);
				param.filepath[pathsize + 2] = 0;
				break;
			}
		}
		for (int i = pathsize + 1; i < strlen(p); i++) {
			param.filename[i - pathsize - 1] = p[i];
			param.filename[i - pathsize] = 0;
		}
		//memcpy(param.filename, p, strlen(p)+1);
	}else if (strcmp("-z", lastparam) == 0) {
		param.SplitSize=Str2Int(p);
	}else if (strcmp("-t", lastparam) == 0) {
		int threads = Str2Int(p);
		if (threads > 32||threads<1) {
			printf("thread num out of range!\n");
			return 0;
		}
		param.threadnum = threads;
	}
}
//检查参数正确性
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
	//GetProcessorCoreCount();
	if (param.work_mode == 's') {
		if(strlen(param.filename) == 0 || param.ip[0] == '*') return 0;
		if (param.threadnum == 0) {
			param.threadnum = GetProcessorCoreCount();
		}
	}
	return 1;
}

int main(int argc, char *argv[]) {

	if (argc < 3) {
		printf("maybe you are input error\n");
		help();
		return 0;
	}
	for (int i = 1; i < argc; i++) {
		//printf("%d %s\n",i,argv[i]);
		if (dealwith_param(argv[i]) == 0) {
			help();
			return 0;
		}
	}
	if (check_param() == 0) {
		help();
		return 0;
	}
	if (param.work_mode == 'r') {	//receiver
		zrecv(&param);
	}
	else if (param.work_mode == 's') {
		allsend(&param);
	}
	return 0;

}