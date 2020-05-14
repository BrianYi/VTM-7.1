#include "KHeader.h"

namespace K 
{

  // BOOL KObject::CreateEx( DWORD dwExStyle, TCHAR* lpszClass, TCHAR* lpszName, DWORD dwStyle, int x, int y, int nWidth, int nHeight, HWND hParent, HMENU hMenu, HINSTANCE hInst )
  // {
  //   if ( !RegisterClass( lpszClass, hInst ) )
  //     return false;
  // 
  //   MDICREATESTRUCT mdic;
  //   memset( &mdic, 0, sizeof( mdic ) );
  //   mdic.lParam = (LPARAM)this;
  // 
  //   fWnd = CreateWindowEx( dwExStyle, lpszClass,
  //     lpszName, dwStyle, x, y, nWidth, nHeight,
  //     hParent, hMenu, hInst, &mdic );
  // 
  //   return fWnd != NULL;
  // }

  DWORD KObject::GetWindowWidth()
  {
      ::GetWindowRect( GetWnd(), &fRCWindow );
      return fRCWindow.right - fRCWindow.left;
    
  }

  DWORD KObject::GetWindowHeight()
  {
    ::GetWindowRect( GetWnd(), &fRCWindow );
    return fRCWindow.bottom - fRCWindow.top;
  }

  BOOL KObject::GetWindowRect( LPRECT lpRect )
  {
    BOOL ret = ::GetWindowRect( GetWnd(), &fRCWindow );
    *lpRect = fRCWindow;
    return ret;
  }

  DWORD KObject::GetClientWidth()
  {
    ::GetClientRect( GetWnd(), &fRCClient );
    return fRCClient.right - fRCClient.left;
  }

  DWORD KObject::GetClientHeight()
  {
    ::GetClientRect( GetWnd(), &fRCClient );
    return fRCClient.bottom - fRCClient.top;
  }

  BOOL KObject::GetClientRect(LPRECT lpRect)
  {
    BOOL ret = ::GetClientRect( GetWnd(), &fRCClient );
    *lpRect = fRCClient;
    return ret;
  }

  BOOL KObject::MoveWindow( int x, int y, int cx, int cy, BOOL bRepaint )
  {
    BOOL ret = ::MoveWindow( GetWnd(), x, y, cx, cy, bRepaint );
    UpdateWindowSizeInfo();
    return ret;
  }

  BOOL KObject::SetIcon( UINT uIconId )
  {
    HICON hIcon = (HICON)::LoadImage( KUtils::GetInstance(), MAKEINTRESOURCE( uIconId ), IMAGE_ICON,
      (::GetSystemMetrics( SM_CXICON ) + 15) & ~15, (::GetSystemMetrics( SM_CYICON ) + 15) & ~15,
      LR_DEFAULTCOLOR );
    assert( hIcon );
    return (BOOL)::SendMessage( GetWnd(), WM_SETICON, (WPARAM)TRUE, (LPARAM)hIcon );
  }

  BOOL KObject::SetWindowText( const TCHAR *szText )
  {
    ::lstrcpy( fWindowText, szText );
    return ::SetWindowText( GetWnd(), fWindowText );
  }

  INT32 KObject::GetWindowText( TCHAR* szBuffer, const int& iBufferCount )
  {
    return ::GetWindowText( GetWnd(), szBuffer, iBufferCount );
  }

  // BOOL KObject::Create( HWND hwndParent, TCHAR* pstrName, DWORD dwStyle, DWORD dwExStyle, const RECT rc, HMENU hMenu )
  // {
  //   if ( !RegisterClass( _T( "KObject" ), KUtils::GetInstance() ) )
  //     return FALSE;
  // 
  //   fWnd = CreateWindowEx( dwExStyle, )
  // }

  WPARAM KObject::MessageLoop( void )
  {
    MSG msg;

    while ( GetMessage( &msg, NULL, 0, 0 ) )
    {
      /*if ( GetMessage( &msg, NULL, 0, 0 ) )*/
      {
        if ( msg.message == WM_QUIT )
          break;
        TranslateMessage( &msg );
        DispatchMessage( &msg );
      }
    }

    return msg.wParam;
  }

  BOOL KObject::RegisterWindowMessage( UINT uMsg )
  {
    assert( uMsg >= WM_USER );
    assert( fMsgCallback.count( uMsg ) == 0 );
    fMsgCallback.insert(uMsg);
    return TRUE;
  }

  KObject::KObject( void ) :
    fWnd(NULL),
    fInst(KUtils::GetInstance()),
    fUUID(KUtils::GenUUID()),
    fUserData(NULL),
    fDefWndProc(NULL),
    fAlignment(AlignTop|AlignLeft)
 //   fBackgroundColor(RGB(255,255,255))
  {
    wsprintf( fUUIDStr, "%I64u", fUUID );
    ::ZeroMemory( fWindowText, sizeof( fWindowText ) );
    KUtils::SetWindow( this );
  }

  KObject::KObject( const UINT64& uUUID ):
    fWnd( NULL ),
    fInst( KUtils::GetInstance() ),
    fUUID( uUUID ),
    fUserData( NULL ),
    fDefWndProc( NULL ),
    fAlignment( AlignTop | AlignLeft )
  {
    wsprintf( fUUIDStr, "%I64u", fUUID );
    ::ZeroMemory( fWindowText, sizeof( fWindowText ) );
    KUtils::SetWindow( this );
  }

//   HRESULT KObject::CommonMDIChildProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
//     HMENU hMenu, int nWindowMenu )
//   {
//     switch ( uMsg )
//     {
//       case WM_NCDESTROY:
//         SetWindowLongPtr( hWnd, GWLP_USERDATA, 0 );
//         delete this;
//         return 0;
// 
//       case WM_MDIACTIVATE:
//       {
//         HWND hParentWnd = GetParent( hWnd );
//         if ( lParam == (LPARAM)hWnd )
//           SendMessage( hParentWnd, WM_MDISETMENU, (WPARAM)hMenu,
//             (LPARAM)GetSubMenu( hMenu, nWindowMenu ) );
//         SendMessage( GetParent( hParentWnd ), WM_USER, lParam != (LPARAM)hWnd, 0 );
//         return 0;
//       }
// 
//       default:
//         break;
//     }
//     return DefMDIChildProc( hWnd, uMsg, wParam, lParam );
//   }

 

  HWND KObject::CreateEx( HWND hwndParent, const TCHAR* lpWindowName, DWORD dwStyle, DWORD dwExStyle, int x /*= CW_USEDEFAULT*/, int y /*= CW_USEDEFAULT*/, int cx /*= CW_USEDEFAULT*/, int cy /*= CW_USEDEFAULT*/, HMENU hMenu /*= NULL */ )
  {
    if ( !RegisterClass( GetClassName() ) )
      assert( false );

    fWnd = CreateWindowEx( dwExStyle, GetClassName(), lpWindowName, dwStyle, x, y,
      cx, cy,
      hwndParent, hMenu, fInst, this );

    UpdateWindowSizeInfo();

    SetAlignment( fAlignment );
    
    assert( fWnd );
    return fWnd;
  }

  int KObject::SetAlignment( int align )
  {
    HWND hParent = GetParent();
    if ( !hParent )
      return FALSE;
    RECT rcParentClient;
    ::GetClientRect( hParent, &rcParentClient );
    return SetAlignment( rcParentClient, align );
  }

  int KObject::SetAlignment( const RECT& rcDest, int align )
  {
    HWND hParent = GetParent();
    if ( !hParent )
      return FALSE;
    RECT rcParentClient;
    ::GetClientRect( hParent, &rcParentClient );
    POINT ptLT{ rcDest.left + 1, rcDest.top + 1 };
    POINT ptRB{ rcDest.right - 1,rcDest.bottom - 1 };
    if ( !(PtInRect( &rcParentClient, ptLT ) && PtInRect( &rcParentClient, ptRB )) )
      return FALSE;

    int cxDest = rcDest.right - rcDest.left;
    int cyDest = rcDest.bottom - rcDest.top;

    int cx = GetWindowWidth();
    int cy = GetWindowHeight();

    int x = rcDest.left;
    int y = rcDest.top;

    fAlignment = 0;
    if ( align & AlignLeft )
    {
      fAlignment |= AlignLeft;
      x = rcDest.left;
    }
    if ( align & AlignRight )
    {
      fAlignment |= AlignRight;
      x = cxDest - cx;
    }
    if ( align & AlignHCenter )
    {
      fAlignment |= AlignHCenter;
      x = (cxDest - cx) / 2;
    }
    if ( align & AlignTop )
    {
      fAlignment |= AlignTop;
      y = rcDest.top;
    }
    if ( align & AlignBottom )
    {
      fAlignment |= AlignBottom;
      y = cyDest - cy;
    }
    if ( align & AlignVCenter )
    {
      fAlignment |= AlignVCenter;
      y = (cyDest - cy) / 2;
    }
    MoveWindow( x, y, cx, cy, TRUE );
    return fAlignment;
  }

  LRESULT KObject::WndProc( UINT uMsg, WPARAM wParam, LPARAM lParam )
  {
    switch ( uMsg )
    {
      HANDLE_WM_MSG( WM_CREATE, wParam, lParam, OnCreate );
      HANDLE_WM_MSG( WM_TIMER, wParam, lParam, OnTimer );
      HANDLE_WM_MSG( WM_SIZE, wParam, lParam, OnSize );
      HANDLE_WM_MSG( WM_PAINT, wParam, lParam, OnPaint );
      HANDLE_WM_MSG( WM_DESTROY, wParam, lParam, OnDestroy );
      HANDLE_WM_MSG( WM_ERASEBKGND, wParam, lParam, OnEraseBkgnd );
      HANDLE_WM_MSG( WM_CONTEXTMENU, wParam, lParam, OnContextMenu );
      HANDLE_WM_MSG( WM_COMMAND, wParam, lParam, OnCommand );
      HANDLE_WM_MSG( WM_NOTIFY, wParam, lParam, OnNotify );
      HANDLE_WM_MSG( WM_CLOSE, wParam, lParam, OnClose );
      HANDLE_WM_MSG( WM_QUERYENDSESSION, wParam, lParam, OnQueryEndSession );
      default:
      {
        if ( fMsgCallback.count( uMsg ) )
          return OnCustom( uMsg, wParam, lParam );
        break;
      }
    }
    return DefWindowProc( uMsg, wParam, lParam );
  }


  LRESULT CALLBACK KObject::_WndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
  {
    KObject* pBase = nullptr;
    if ( uMsg == WM_NCCREATE )
    {
      pBase = (KObject*)(MDICREATESTRUCT*)(((LPCREATESTRUCT)lParam)->lpCreateParams);
      pBase->fWnd = hWnd;
      ::SetWindowLongPtr( hWnd, GWLP_USERDATA, (LPARAM)pBase );
    }
    else
      pBase = (KObject*)::GetWindowLongPtr( hWnd, GWLP_USERDATA );
    
    if ( pBase )
      return pBase->WndProc( uMsg, wParam, lParam );
    else
      return ::DefWindowProc( hWnd, uMsg, wParam, lParam );
  }

  VOID KObject::UpdateWindowSizeInfo()
  {
     ::GetWindowRect( fWnd, &fRCWindow );
     ::GetClientRect( fWnd, &fRCClient );
  }

  BOOL KObject::RegisterClass( const TCHAR* lpszClass )
  {
    assert( lpszClass );
    WNDCLASSEX wce;
    memset( &wce, 0, sizeof( wce ) );
    if ( GetClassInfoEx( fInst, lpszClass, &wce ) )
    {
      if ( !fDefWndProc)
      {
        if ( lpszClass[0] == _T( 'S' ) && lpszClass[1] == _T( '_' ) )
        {
          if ( !GetClassInfoEx( fInst, &lpszClass[2], &wce ) )
          {
            assert( false );
            return FALSE;
          }
          fDefWndProc = wce.lpfnWndProc;
        }
        else
          fDefWndProc = ::DefWindowProc;
      }
      return TRUE;
    }

    if ( lpszClass[0] == _T( 'S' ) && lpszClass[1] == _T( '_' ) )
    {
      if ( !GetClassInfoEx( fInst, &lpszClass[2], &wce ) )
      {
        assert( false );
        return FALSE;
      }
      fDefWndProc = wce.lpfnWndProc;
      wce.cbSize = sizeof( WNDCLASSEX );
      wce.hInstance = fInst;
      wce.lpszClassName = GetClassName();
      wce.lpfnWndProc = _WndProc;
      if ( !RegisterClassEx( &wce ) )
      {
        assert( false );
        return FALSE;
      }
      return TRUE;
    }
    fDefWndProc = ::DefWindowProc;
    wce.cbSize = sizeof( WNDCLASSEX );
    wce.lpfnWndProc = _WndProc;
    wce.hInstance = fInst;
    wce.hCursor = LoadCursor( NULL, IDC_ARROW );
    wce.hbrBackground = (HBRUSH)GetStockObject( WHITE_BRUSH );
    wce.lpszClassName = GetClassName();
    if ( !RegisterClassEx( &wce ) )
    {
      assert( false );
      return FALSE;
    }

    return TRUE;
  }

};