// LargeFile-SEMt-transmission.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//
#include <zmq.h>
#include <pthread.h>
#include "zhelpers.h"
#include "server.h"
#include "client.h"
#include"fConfig.h"
#include"sha256.h"
#include "aes.h"
#include"aes.h"
//#include"clientSend.h"
//#include "serverRecv.h"
argv_info param; 
int Str2Int(char * buff) {
	int num = 0;
	int index = 0;
	for (; buff[index] >= '0' && buff[index] <= '9'; index++) {
		num = num * 10 + buff[index] - '0';
	}
	return num;
}

char lastparam[3]; void split_ip_port(char *p) {
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
//return 0:error, 1: -x, 2: get parameter end;
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
void test(int tesa) {
	DWORD outlen;
	unsigned char in[] = { "asdfgh" };
	unsigned char out_data[32];
	unsigned char in_data[32];
	UCHAR key[] = "qwerasdfzxcvtyuiqwerasdfzxcvtyui";
	InitializePrivateKey(32, key);
	outlen = AesEncrypt(in, 6, out_data);//DWORD OnAesEncrypt(LPVOID InBuffer,DWORD InLength,LPVOID OutBuffer)
	phex(out_data);
	Sleep(2000);
	outlen = AesDecrypt(out_data, 16, in_data);//DWORD OnAesUncrypt(LPVOID InBuffer,DWORD InLength,LPVOID OutBuffer)
	printf("1 %s\n", in_data);
}void test2(int tesa) {
	Sleep(1000);
	DWORD outlen;
	unsigned char in[] = { "asdfgh22" };
	unsigned char out_data[32];
	unsigned char in_data[32];
	UCHAR key[] = "qwerasdfzxcvtyuiqwerasdfzxcvtyui";
	InitializePrivateKey(32, key);
	outlen = AesEncrypt(in, 6, out_data);//DWORD OnAesEncrypt(LPVOID InBuffer,DWORD InLength,LPVOID OutBuffer)
	phex(out_data);
	outlen = AesDecrypt(out_data, 16, in_data);//DWORD OnAesUncrypt(LPVOID InBuffer,DWORD InLength,LPVOID OutBuffer)
	printf("2 %s\n", in_data);
}
int main(int argc, char *argv[]) {
#if 0
	char buffer[]="Kk7NhVtZFXgjoES6HrFxonUFcxaw4Ay0e0weyDx8u7g0vAZa3BXAQcQcGaKRuXpYr63iogafiMIyTWMWfTHJFEUdQvqYYeHctA745EkUObuW05cIdXpvdqNslcjd2QoJpFr4kGYV76CyPja7px1d5rUNlcAHhXoSs9yofMpO2XUm6zai6neGAAMbCra7g92B82QRaFDDZ25MGaxZfa2Qd3YHmYLPKnYVEKHrdwskrtiqtUy3fK3im2oaLluegwdJfc3sbPsGAigOuKFy90EZVjAS2xAoFj0scBCbC2qoYYl7OdqHPVHnaYvzqnsYL4B2YtwIL0KvytzNnofzpsKsl2VuThDrj1MeBi8vsGByhMNXCrs6LytKzpv5tfEU2sg34DLTgiz2ZTMOLvzufWSjMaJopI8KcymzqWTTruXlmab0Obd61cOOnUay3cmoMxrM0DnnI2FUG2lxuZQmyZ4VvsZspjuUQgtlspSC9JRDUmIDkXYOr7BochGXfuHJzO1b41osWbHkB4aMRZqabNEr6xMID4KPfgzWGlalKcVQUe26jDoO40ZAykzxtF1E4JnIe8m51ZRfAj7IHP5LA2JbV9wDnQ56BprgBCW3292OdvhIPcxHNCLOFAOgf7U0g8HGIHaU5pWvgeruqFDk1Ja9bMS56KzWmbbnzjnlyGPJtvwzmJ1Rz0YNlNbA7MBLjzQXhmZRFZ4orJW3bnQwLfIJOOyGpXXa8qBFlezJUl71qJZ3Vlmt4lhGqYOGCWkR8Ul6JOw9R5TPfWnZX7nm7FeXbfr9zIeRIhRgH56oeJDjOavLK3shpfJxoZWUSRL6oIX1UDJT5U3B7wV86CMAIkOWLbTHrNhgzBfapPlEwBh9x2kZHqUTz4hgQNv22X4roca1gZKTDGVrUi1NGB9AmIrEAdnJJdgRKfR8gTEkeg2rr8BC4lTQpT40doXfVJSj382zQNAp0AYgy6LcUBO4o8SzEjiagLPQraTTKDvIQDF1bMUi5UZ3cKX1cg0Wz39Cbln";
	char key[] = "6ObE25XBPnzMAUM9bH29sLkp9q0LLAN8Ic9RCjIR3P39unEG0SIfWVfalzSwjBxQ";
	printf("buffer len %d  key len %d\n", strlen(buffer), strlen(key));
	int len = 8;//dex
	UCHAR *dkey = (char*)malloc(len + 1);
	memcpy(dkey, key, len + 1);
	dkey[len + 1] = 0;
	int size = sizeof(buffer);
	unsigned char *out = (unsigned char*)malloc(size);
	memset(out, 0, size);

	printf("before encrypt: \n");
	phex(buffer);
	phexe(buffer);
	des_encrypt(key, len, buffer, out,size/8);
	printf("after encrypt: \n\n");
	phex(out);
	phexe(out);
	des_decrypt(key, len, out, buffer, size/8);
	printf("after decrypt: \n\n" );
	phex(buffer);
	phexe(buffer);

	des_encrypt(key, len*2, buffer, out, size / 8);
	printf("after 3encrypt: \n\n");
	phex(out);
	phexe(out);
	des_decrypt(key, len*2, out, buffer, size / 8);
	printf("after 3decrypt: \n\n");
	phex(buffer);
	phexe(buffer);
	return 0;
#endif
	//pthread_t asd,asd2;
	//int pa = 123;
	//int ret3 = pthread_create(&asd, NULL, test, pa);
	//if (ret3 != 0)
	//{
	//	printf("pthread_create error: error_code=%d", ret3);
	//}
	//int pa2 = 13;
	//int ret = pthread_create(&asd2, NULL, test2, pa2);
	//if (ret != 0)
	//{
	//	printf("pthread_create error: error_code=%d", ret);
	//}
	//Sleep(6000);
	//return 0;
	//DWORD outlen;
	//unsigned char in[] = "a8sczvvasdwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczasczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pIzvvasdvwag1541asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv8E1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45asczvvasdvwag15416tn51gbx-=23t62&*(YH)(nifnsdv89pINFE1651tweeffOIUHfs8uw45";
	//;
	//printf("in len %d size %d\n", strlen(in),sizeof(in));
	//unsigned char out_data[10240];
	//unsigned char in_data[10240];
	//UCHAR key[] = "qwerasdfzxcvtyuiqwerasdfzxcvtyui";
	//InitializePrivateKey(32, key);
	//outlen = AesEncrypt(in, 10240, out_data);//DWORD OnAesEncrypt(LPVOID InBuffer,DWORD InLength,LPVOID OutBuffer)
	//phex(out_data);
	//printf("outdata len %d\n",outlen);
	//outlen = AesDecrypt(out_data, outlen, in_data);//DWORD OnAesUncrypt(LPVOID InBuffer,DWORD InLength,LPVOID OutBuffer)
	//printf("o %s", in_data);
	////UART4_Send_Data(in_data,6);
	//return 0;
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
	//printf("mode: %c  ip: %s  port: %s  crypt: %d ", param.work_mode, param.ip, param.port, param.crypt_mode);
	//if(param.passwd[0]!=0) printf("passwd: %s ", param.passwd);
	//if (param.filename[0] != 0) printf("filename: %s  path: %s\n", param.filename, param.filepath);
	if (param.work_mode == 'r') {	//receiver
		zrecv(&param);
	}
	else if (param.work_mode == 's') {
		allsend(&param);
	}
	return 0;
#if 0
	pthread_t pRouter;
	int ret3 = pthread_create(&pRouter, NULL, zrecv, &param);
	if (ret3 != 0)
	{
		printf("pthread_create error: error_code=%d", ret3);
	}
	allsend(&param);
	pthread_join(pRouter,NULL);
#endif
}