#ifndef _CLIENT_H
#define _CLIENT_H_
#include <zmq.h>
#include"zhelpers.h"
#include"fConfig.h"
#include"sha256.h"
//extern void zsend();
void allsend(argv_info * info);
#endif