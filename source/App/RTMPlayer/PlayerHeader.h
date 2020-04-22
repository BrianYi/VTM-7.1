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

class RtmpWindow
{
  HWND fHwnd;
  std::string fApp;
  int32_t fTimebase;
  uint64_t fMenuId;
  PriorityQueue fPriQue;
  bool fDestroyed;
public:
  RtmpWindow( HWND win, std::string app, int32_t timebase, uint64_t menuid ):
    fHwnd(win),
    fApp(app),
    fTimebase(timebase),
    fMenuId(menuid),
    fDestroyed(false)
  {}
  ~RtmpWindow()
  {
    DestroyWindow( fHwnd );
    while ( !fPriQue.empty() )
    {
      Packet *packet = fPriQue.top();
      fPriQue.pop();
      delete packet;
    }
  }
  PriorityQueue& pri_queue() { return fPriQue; }
  void set_win( HWND win ) { fHwnd = win; }
  HWND win() { return fHwnd; }
  void set_app( std::string app ) { fApp = app; }
  std::string app() { return fApp; }
  void set_timebase( int32_t timebase ) { fTimebase = timebase; }
  int32_t timebase() { return fTimebase; }
  void set_menuid( uint64_t menuid ) { fMenuId = menuid; }
  uint64_t menuid() { return fMenuId; }
  void set_destroyed() { fDestroyed = true; }
  bool destroyed() { return fDestroyed; }
};
