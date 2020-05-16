#pragma execution_character_set("utf-8")
#pragma once
#include "Packet.h"

//#define SERVER_IP			      "39.105.201.141"
#define SERVER_IP			      "127.0.0.1"
#define SERVER_PORT			    5566
#define MAX_CONNECTION_NUM	8

#if _DEBUG
#define DEBUG_LOCAL_VIDEO_TEST  0
#define DEBUG_NO_ENCODING       1
#define DEBUG_SEND_TO_MYSELF    0
//#define DEBUG_DOUBLE_CHECK      0
#endif


// #if _WIN32
// #define msleep(ms)	Sleep(ms)
// #else
// #define ntohll		be64toh
// #define htonll		htobe64
// #define msleep(ms)	usleep(1000 * ms)
// #endif