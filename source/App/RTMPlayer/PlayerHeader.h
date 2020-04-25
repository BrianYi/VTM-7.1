#pragma once
#include "PlatformHeader.h"
#include "Packet.h"
#include <mutex>

#define SERVER_IP			      0x7F000001
#define SERVER_PORT			    5566
#define MAX_CONNECTION_NUM	8

#if _DEBUG
#define DEBUG_LOCAL_VIDEO_TEST  0
#define DEBUG_NO_ENCODING       1
#define DEBUG_SEND_TO_MYSELF    0
#define DEBUG_DOUBLE_CHECK      0
#endif

// new connection, need to open a new window
#define WM_NEW_CONNECTION		WM_USER+1
// new online user, don't need to open a new window
#define WM_NEW_ONLINEUSER		WM_USER+2
#define WM_DEL_ONLINEUSER		WM_USER+3
#define WM_NEW_YUV_FRAME    WM_USER+4

#if defined _M_IX86
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_IA64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='ia64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif

class cmp
{
public:
  bool operator()( Packet* p1, Packet* p2 )
  {
    if ( p1->timestamp() > p2->timestamp() )
      return true;
    else if ( p1->timestamp() == p2->timestamp() )
      return p1->seq() > p2->seq();
    return false;
  }
};

struct PARAMS
{
  int flags;
  Packet *ptrPacket;
};

struct Frame
{
  //int type;
  int64_t timestamp;
  int32_t size;
  char *data;
};

class PriorityQueue
{
  std::priority_queue<Packet*, std::vector<Packet*>, cmp> encPacketQueue;
  std::mutex fMx;
public:
  void push( Packet *packet )
  {
    std::unique_lock<std::mutex> lock( fMx );
    encPacketQueue.push( packet );
  }
  Packet *top()
  {
    std::unique_lock<std::mutex> lock( fMx );
    return encPacketQueue.top();
  }
  void pop()
  {
    std::unique_lock<std::mutex> lock( fMx );
    encPacketQueue.pop();
  }
  bool empty()
  {
    return encPacketQueue.empty();
  }
};

