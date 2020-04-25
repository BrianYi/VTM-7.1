#pragma once

#include "DecoderApp/DecApp.h"
#include "program_options_lite.h"
#include "PlayerHeader.h"
#include "RtmpUtils/Log.h"

class RtmpWindow
{
  HWND fHwnd;
  std::string fApp;
  int32_t fTimebase;
  uint32_t fMenuId;
  PriorityQueue fPriQue;
  bool fLostConnection;
  HANDLE fHDecoderThread;
public:
  RtmpWindow( HWND win, std::string app, int32_t timebase, uint32_t menuid );
  ~RtmpWindow();
  PriorityQueue& pri_queue() { return fPriQue; }
  HWND win() { return fHwnd; }
  void set_app( std::string app ) { fApp = app; }
  std::string app() { return fApp; }
  void set_timebase( int32_t timebase ) { fTimebase = timebase; }
  int32_t timebase() { return fTimebase; }
  void set_menuid( uint32_t menuid ) { fMenuId = menuid; }
  uint32_t menuid() { return fMenuId; }
  void set_lost() { fLostConnection = true; }
  bool lost() { return fLostConnection; }
};
