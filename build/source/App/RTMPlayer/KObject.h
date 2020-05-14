#pragma once
typedef LRESULT( *KWNDPROC )(UINT, WPARAM, LPARAM);

namespace K 
{
#define UI_WNDSTYLE_CONTAINER  (0)
#define UI_WNDSTYLE_FRAME      (WS_VISIBLE | WS_OVERLAPPEDWINDOW)
#define UI_WNDSTYLE_CHILD      (WS_VISIBLE | WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN)
#define UI_WNDSTYLE_DIALOG     (WS_VISIBLE | WS_POPUPWINDOW | WS_CAPTION | WS_DLGFRAME | WS_CLIPSIBLINGS | WS_CLIPCHILDREN)

#define UI_WNDSTYLE_EX_FRAME   (WS_EX_WINDOWEDGE)
#define UI_WNDSTYLE_EX_DIALOG  (WS_EX_TOOLWINDOW | WS_EX_DLGMODALFRAME)

#define UI_CLASSSTYLE_CONTAINER  (0)
#define UI_CLASSSTYLE_FRAME      (CS_VREDRAW | CS_HREDRAW)
#define UI_CLASSSTYLE_CHILD      (CS_VREDRAW | CS_HREDRAW | CS_DBLCLKS | CS_SAVEBITS)
#define UI_CLASSSTYLE_DIALOG     (CS_VREDRAW | CS_HREDRAW | CS_DBLCLKS | CS_SAVEBITS)

#define HANDLE_WM_MSG(uMsg, wParam, lParam, fn) \
    case (uMsg): return fn(uMsg, wParam, lParam);

#define DEFINE_WM_MSG_HANDLER(fn) \
virtual LRESULT fn(UINT uMsg, WPARAM wParam, LPARAM lParam) \
{ return DefWindowProc(uMsg, wParam, lParam); }

  class KObject
  {
  public:
    enum Alignment
    {
      AlignLeft = 1 << 0,
      AlignRight = 1 << 1,
      AlignHCenter = 1 << 2,
      AlignTop = 1 << 3,
      AlignBottom = 1 << 4,
      AlignVCenter = 1 << 5,
    };
    KObject( void );
    KObject( const UINT64& uUUID );
    virtual ~KObject()
    {}
    virtual const TCHAR* GetClassName() = 0;
    BOOL UpdateWindow( void ) { return ::UpdateWindow( fWnd ); }
    HWND CreateEx( HWND hwndParent, const TCHAR* pstrWindowName, DWORD dwStyle,
      DWORD dwExStyle, int x = CW_USEDEFAULT, int y = CW_USEDEFAULT, int cx = CW_USEDEFAULT, int cy = CW_USEDEFAULT, HMENU hMenu = NULL );
    int SetAlignment( int align );
    int SetAlignment( const RECT& rc, int align );
    int GetAlignment() { return fAlignment; }
    DWORD GetWindowWidth();
    DWORD GetWindowHeight();
    BOOL GetWindowRect( LPRECT lpRect );
    DWORD GetClientWidth();
    DWORD GetClientHeight();
    BOOL GetClientRect( LPRECT lpRect );
    BOOL MoveWindow( int x, int y, int cx, int cy, BOOL bRepaint );
    BOOL SetIcon( UINT uIconId );
    BOOL SetWindowText( const TCHAR *szText );
    INT32 GetWindowText(TCHAR* szBuffer, const int& iBufferCount);
    HWND GetWnd() const { return fWnd; }
    HWND GetParent() { return ::GetParent( GetWnd() ); }
    HMENU GetMenu() { return ::GetMenu( GetWnd() ); }
    UINT64 GetUUID() { return fUUID; }
    const TCHAR* GetUUIDStr() { return fUUIDStr; }
    LRESULT SendMessage( UINT Msg, WPARAM wParam, LPARAM lParam ) 
    { return ::SendMessage( GetWnd(), Msg, wParam, lParam ); }
    LRESULT PostMessage( UINT Msg, WPARAM wParam, LPARAM lParam )
    { return ::PostMessage( GetWnd(), Msg, wParam, lParam ); }
    void SetUserData( void *data ) { fUserData = data; }
    void *GetUserData() { return fUserData; }
    BOOL InvalidateRgn( HRGN hRgn, BOOL bErase)
    { return ::InvalidateRgn( GetWnd(), hRgn, bErase ); }
    BOOL InvalidateRect( const RECT* lpRect, BOOL bErase )
    { return ::InvalidateRect( GetWnd(), lpRect, bErase ); }
    BOOL ValidateRect(const RECT *lpRect)
    { return ::ValidateRect( GetWnd(), lpRect ); }
    BOOL SetWindowPos(HWND hWndInsertAfter, int x, int y, int cx, int cy, UINT uFlags)
    { return ::SetWindowPos( GetWnd(), hWndInsertAfter, x, y, cx, cy, uFlags ); }
    BOOL SetMenu( HMENU hMenu )
    { return ::SetMenu( fWnd, hMenu ); }
    LONG_PTR GetClassLongPtr( int nIndex )
    { return ::GetClassLongPtr( fWnd, nIndex ); }
    LONG_PTR SetClassLongPtr( int nIndex, LONG_PTR dwNewLong )
    { return ::SetClassLongPtr( fWnd, nIndex, dwNewLong );}
    LONG_PTR GetWindowLongPtr(int nIndex)
    { return ::GetWindowLongPtr( fWnd, nIndex ); }
    LONG_PTR SetWindowLongPtr( int nIndex, LONG_PTR dwNewLong )
    { return ::SetWindowLongPtr( fWnd, nIndex, dwNewLong ); }
    BOOL Hide() { return ShowWindow( SW_HIDE ); }
    BOOL Show() { return ShowWindow( SW_SHOW ); }
    WPARAM MessageLoop( void );
    LRESULT DefWindowProc( UINT Msg, WPARAM wParam, LPARAM lParam )
    { return ::CallWindowProc( fDefWndProc, GetWnd(), Msg, wParam, lParam ); }
    BOOL RegisterWindowMessage( UINT uMsg );
  protected:
    DEFINE_WM_MSG_HANDLER( OnCreate );
    DEFINE_WM_MSG_HANDLER( OnTimer );
    DEFINE_WM_MSG_HANDLER( OnSize );
    DEFINE_WM_MSG_HANDLER( OnPaint );
    DEFINE_WM_MSG_HANDLER( OnDestroy );
    DEFINE_WM_MSG_HANDLER( OnEraseBkgnd );
    DEFINE_WM_MSG_HANDLER( OnContextMenu );
    DEFINE_WM_MSG_HANDLER( OnCommand );
    DEFINE_WM_MSG_HANDLER( OnNotify );
    DEFINE_WM_MSG_HANDLER( OnClose );
    DEFINE_WM_MSG_HANDLER( OnQueryEndSession );
    DEFINE_WM_MSG_HANDLER( OnCustom );
  private:
    virtual LRESULT WndProc( UINT uMsg, WPARAM wParam, LPARAM lParam );
    BOOL RegisterClass( const TCHAR* lpszClass );
    static LRESULT CALLBACK _WndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
    VOID UpdateWindowSizeInfo();
    BOOL ShowWindow( int nCmdShow ) { return ::ShowWindow( fWnd, nCmdShow ); }
  private:
    ::WNDPROC fDefWndProc;
    HWND fWnd;
    HINSTANCE fInst;
    TCHAR fWindowText[MAX_PATH];
    UINT64 fUUID;
    TCHAR fUUIDStr[64];
    void *fUserData;
    RECT fRCWindow;
    RECT fRCClient;
    int fAlignment;
//    DWORD fBackgroundColor;
    std::unordered_set<UINT> fMsgCallback;
  };

};