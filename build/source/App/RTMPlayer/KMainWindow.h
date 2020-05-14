#pragma once
#include "KHeader.h"
#include "KToolbar.h"
using namespace K;

class KMainWindow :
  public KObject
{
public:
  KMainWindow(): KObject()
  {
    RegisterWindowMessage( WM_ONLINE );
    RegisterWindowMessage( WM_OFFLINE );
    RegisterWindowMessage( WM_REQUEST );
    RegisterWindowMessage( WM_REFUSED );
    RegisterWindowMessage( WM_ACCEPT );
  }
  virtual ~KMainWindow()
  {}
  virtual const TCHAR* GetClassName() { return _T( "KMainWindow" ); }
  void UpdateOnlineUsers();
protected:
  virtual LRESULT OnCreate( UINT uMsg, WPARAM wParam, LPARAM lParam );
  virtual LRESULT OnSize( UINT uMsg, WPARAM wParam, LPARAM lParam );
  virtual LRESULT OnContextMenu( UINT uMsg, WPARAM wParam, LPARAM lParam );
  virtual LRESULT OnCommand( UINT uMsg, WPARAM wParam, LPARAM lParam );
  virtual LRESULT OnPaint( UINT uMsg, WPARAM wParam, LPARAM lParam );
  virtual LRESULT OnDestroy( UINT uMsg, WPARAM wParam, LPARAM lParam );
  virtual LRESULT OnNotify( UINT uMsg, WPARAM wParam, LPARAM lParam );
  virtual LRESULT OnClose( UINT uMsg, WPARAM wParam, LPARAM lParam );

  // custome message
  virtual LRESULT OnCustom( UINT uMsg, WPARAM wParam, LPARAM lParam );
  virtual LRESULT OnOnline( UINT uMsg, WPARAM wParam, LPARAM lParam );
  virtual LRESULT OnOffline( UINT uMsg, WPARAM wParam, LPARAM lParam );
  virtual LRESULT OnRequest( UINT uMsg, WPARAM wParam, LPARAM lParam );
  virtual LRESULT OnRefused( UINT uMsg, WPARAM wParam, LPARAM lParam );
  virtual LRESULT OnAccept( UINT uMsg, WPARAM wParam, LPARAM lParam );
protected:
  static INT_PTR CALLBACK ConfirmDlgProc( HWND hDlg, UINT message,
    WPARAM wParam, LPARAM lParam );
  static INT_PTR CALLBACK RequestDlgProc( HWND hDlg, UINT message,
    WPARAM wParam, LPARAM lParam );
  static INT_PTR CALLBACK RefusedDlgProc( HWND hDlg, UINT message,
    WPARAM wParam, LPARAM lParam );
  static INT_PTR CALLBACK ExitDlgProc( HWND hDlg, UINT message,
    WPARAM wParam, LPARAM lParam );
  static INT_PTR CALLBACK AboutDlgProc( HWND hDlg, UINT message,
    WPARAM wParam, LPARAM lParam );

  static unsigned CALLBACK EncoderThread( void *arg );
#if DEBUG_LOCAL_VIDEO_TEST
  HBITMAP NextFrame( HDC hdc, HANDLE hFile, DWORD cxOrigPic, DWORD cyOrigPic );
#endif
private:
  KToolbar fToolbar;
  KToolbar fOnlineUsers;
  HANDLE fEncoderThread;
  std::vector<UINT64> fRtmpWindowArry;
};

