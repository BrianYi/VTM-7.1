#include "KHeader.h"
#include "event2/event.h"
#include "PlayerHeader.h"

void KMainWindow::UpdateOnlineUsers()
{
  RECT rcClient;
  GetClientRect( &rcClient );
  rcClient.bottom -= fToolbar.GetWindowHeight();
  fOnlineUsers.SetAlignment( rcClient, AlignHCenter | AlignVCenter );
}

LRESULT KMainWindow::OnCreate( UINT uMsg, WPARAM wParam, LPARAM lParam )
{
  HMENU hMenu = CreateMenu();
  //HMENU hMenuPop = CreatePopupMenu();
  AppendMenu( hMenu, MF_SEPARATOR, 0, NULL );
  SetMenu( hMenu );
  // create main window
  this->SetIcon( IDI_PLAYER );

  KUtils::SetMainWindow( this );

  // create toolbar
  HWND hWnd = fToolbar.CreateEx( GetWnd(), NULL,
    TBSTYLE_FLAT | CCS_NODIVIDER | CCS_NORESIZE | WS_CHILD,
    0, 32, 32 );
  fToolbar.SendMessage( TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_DRAWDDARROWS );
  assert( hWnd );
  fEncoderThread = NULL;

  KTBBUTTON tbButtons[] =
  {
  {_T( "解除静音" ), _T( "conf_Wnd/Icon_Toolbar_Voice_Normal.png" ), IDB_TOOLBAR_VOICE, TBSTATE_ENABLED,  BTNS_DROPDOWN, NULL },
  {_T( "开启视频" ), _T( "conf_Wnd/Icon_Toolbar_View_Normal.png" ), IDB_TOOLBAR_VIEW, TBSTATE_ENABLED, BTNS_DROPDOWN},
  {_T( "共享屏幕" ), _T( "conf_Wnd/Icon_Toolbar_Share_Normal.png" ),IDB_TOOLBAR_SHARE, TBSTATE_ENABLED,  BTNS_DROPDOWN, NULL },
  {_T( "邀请" ), _T( "conf_Wnd/Icon_Toolbar_Invite_Normal.png" ),IDB_TOOLBAR_INVITE, TBSTATE_ENABLED, NULL, NULL },
  {_T( "管理成员" ), _T( "conf_Wnd/Icon_Toolbar_Manage_Normal.png" ), IDB_TOOLBAR_MANAGE, TBSTATE_ENABLED, NULL, NULL },
  {_T( "聊天" ), _T( "conf_Wnd/toolbar_icon_chat_on_normal.png" ), IDB_TOOLBAR_CHATON, TBSTATE_ENABLED, NULL, NULL },
  {_T( "表情" ), _T( "conf_Wnd/toolbar_icon_emoji_normal.png" ), IDB_TOOLBAR_EMOJI, TBSTATE_ENABLED, NULL, NULL },
  {_T( "文档" ), _T( "conf_Wnd/Icon_Toolbar_Document_Normal.png" ), IDB_TOOLBAR_DOCUMENT, TBSTATE_ENABLED, NULL, NULL },
  {_T( "设置" ), _T( "conf_Wnd/Icon_Toolbar_More_Normal.png" ), IDB_TOOLBAR_SETTING, TBSTATE_ENABLED, NULL, NULL }
  };
  fToolbar.AddButtons( tbButtons, sizeof( tbButtons ) / sizeof( tbButtons[0] ) );
  //fToolbar.SetPadding( 45, 0 );
  fToolbar.AddButtonImage( IDB_TOOLBAR_VOICE, _T( "conf_Wnd/Icon_Toolbar_Voice_Open_Normal.png" ) );
  fToolbar.AddButtonImage( IDB_TOOLBAR_VIEW, _T( "conf_Wnd/Icon_Toolbar_View_Open_Normal.png" ) );
  fToolbar.SetAlignment( AlignBottom | AlignHCenter );
  fToolbar.Show();

  hWnd = fOnlineUsers.CreateEx( GetWnd(), NULL,
    TBSTYLE_FLAT | CCS_NODIVIDER | CCS_NORESIZE | TBSTYLE_WRAPABLE | WS_CHILD,
    0, 92, 92 );
  //fOnlineUsers.SetPadding( 20, 0 );
  fOnlineUsers.Show();
  assert( hWnd );
  
  // wait dispatcher finished starting
  KUtils::WaitDispatcherStartFinished();

  // 
  SendMessage( WM_ONLINE, GetUUID(), 0 );

  // C->S: NewSession
  KUtils::SubmitWork( EV_WRITE, PacketUtils::new_session_packet( get_timestamp_ms(),
    KUtils::GetMainWindow()->GetUUIDStr() ) );

  return TRUE;
}

LRESULT KMainWindow::OnSize( UINT uMsg, WPARAM wParam, LPARAM lParam )
{
  INT32 cxClient = LOWORD( lParam );
  INT32 cyClient = HIWORD( lParam );
  RECT rcRtmpWindow;
  for ( auto it = fRtmpWindowArry.begin(); it != fRtmpWindowArry.end(); ++it )
  {
    KRtmpWindow *rtmpWindow = (KRtmpWindow *)KUtils::GetWindow( *it );
    assert( lstrcmp(rtmpWindow->GetClassName(), TEXT( "KRtmpWindow" ) ) == 0);
    INT32 cxRtmpWindow = GetClientWidth();
    INT32 cyRtmpWindow = GetClientHeight() - fToolbar.GetWindowHeight();
    rtmpWindow->MoveWindow( 0, 0, cxRtmpWindow, cyRtmpWindow, FALSE );
    rtmpWindow->GetWindowRect( &rcRtmpWindow );
  }

  RECT rcClient, rcTemp;
  GetClientRect( &rcClient );
  rcTemp = rcClient;
  rcTemp.bottom -= fToolbar.GetWindowHeight();
  fOnlineUsers.SetAlignment( rcTemp, AlignHCenter | AlignVCenter );
  fToolbar.SetAlignment( rcClient, AlignBottom | AlignHCenter );

  InvalidateRect( NULL, TRUE );
  return TRUE;
}

LRESULT KMainWindow::OnOnline( UINT uMsg, WPARAM wParam, LPARAM lParam )
{
  UINT64 uUUID = wParam;
  TCHAR szUUIDStr[64];
  wsprintf( szUUIDStr, _T("%I64u"), uUUID );
  
  KTBBUTTON tbbutton{ szUUIDStr, _T( "login/login_head_default@2x.png" ), KUtils::GetCommandID( uUUID ), TBSTATE_ENABLED,  NULL, NULL };
  static DWORD dwPickIcon = 0;
  if (dwPickIcon == 0 )
    tbbutton.szImageFile = _T( "login/splash_icon_wechat_mousedown@2x.png" );
  else
  if (dwPickIcon == 1)  
    tbbutton.szImageFile = _T( "login/splash_icon_wechatwork_mousedown@2x.png" );
  else
    tbbutton.szImageFile = _T( "login/splash_icon_sso_normal@2x.png" );
  dwPickIcon = (dwPickIcon + 1) % 3;

  fOnlineUsers.AddButton( &tbbutton );
  UpdateOnlineUsers();
  return FALSE;
}

LRESULT KMainWindow::OnOffline( UINT uMsg, WPARAM wParam, LPARAM lParam )
{
  /*
     * WPARAM: menuid
     */
  //DeleteMenu( hMenu, wParam, MF_BYCOMMAND );
  UINT64 uUUID = wParam;
  fOnlineUsers.DeleteButton( KUtils::GetCommandID(uUUID) );
  UpdateOnlineUsers();

  // no user, stop encoding
  if ( fOnlineUsers.GetButtonCount() == 1 )
  {
    //TerminateThread( hDispatcherThread, 0 ); don't use this function!
    //g_bIsEncoding = FALSE;
  }
  return TRUE;
}

LRESULT KMainWindow::OnRequest( UINT uMsg, WPARAM wParam, LPARAM lParam )
{
  UINT64 uPeerUUID = wParam;
  INT32 iTimebase = (INT32)lParam;
  TCHAR szPeerUUIDStr[64];
  wsprintf( szPeerUUIDStr, _T("%I64u"), uPeerUUID );

  if ( DialogBoxParam( KUtils::GetInstance(), MAKEINTRESOURCE( IDD_DIALOG_REQUEST ),
    GetWnd(), RequestDlgProc, uPeerUUID ) == IDOK )
  {
      KUtils::SubmitWork( EV_WRITE, PacketUtils::accept_packet( lstrlen(szPeerUUIDStr), GetUUIDStr(), g_timebase,
        szPeerUUIDStr ) );
      fOnlineUsers.Hide();
  }
  else
  {
      KUtils::SubmitWork( EV_WRITE, PacketUtils::refuse_packet( lstrlen( szPeerUUIDStr ), GetUUIDStr(), g_timebase,
        szPeerUUIDStr ) );
      return TRUE;
  }

  KRtmpWindow *rtmpWindow = (KRtmpWindow *)KUtils::GetWindow( uPeerUUID );
  assert( lstrcmp( rtmpWindow->GetClassName(), _T( "KRtmpWindow" )) == 0 );

  rtmpWindow->CreateEx( GetWnd(), NULL, WS_CHILD, 0,
    0, 0, GetClientWidth(), GetClientHeight() - fToolbar.GetWindowHeight());
  rtmpWindow->Show();
  rtmpWindow->SetTimebase( iTimebase );
  fRtmpWindowArry.push_back( uPeerUUID );

  DWORD dwCxClient = GetClientWidth();
  DWORD dwCyClient = GetClientHeight();
  SendMessage( WM_SIZE, 0, MAKELPARAM( dwCxClient, dwCyClient ) );

  // begin encoding
  if ( !fEncoderThread )
  {
    fEncoderThread = (HANDLE)_beginthreadex( NULL, 0, EncoderThread, rtmpWindow, 0, NULL );
  }
  return TRUE;
}

LRESULT KMainWindow::OnRefused( UINT uMsg, WPARAM wParam, LPARAM lParam )
{

  UINT64 uPeerUUID = wParam; 
  DialogBoxParam( KUtils::GetInstance(), MAKEINTRESOURCE( IDD_DIALOG_REFUSED ),
    GetWnd(), RefusedDlgProc, uPeerUUID );
  return TRUE;
}

LRESULT KMainWindow::OnAccept( UINT uMsg, WPARAM wParam, LPARAM lParam )
{
  UINT64 uPeerUUID = wParam;
  INT32 iTimebase = (INT32)lParam;
  TCHAR szPeerUUIDStr[64];
  wsprintf( szPeerUUIDStr, _T( "%I64u" ), uPeerUUID );

  fOnlineUsers.Hide();

  KRtmpWindow *rtmpWindow = (KRtmpWindow *)KUtils::GetWindow( uPeerUUID );
  assert( lstrcmp( rtmpWindow->GetClassName(), _T( "KRtmpWindow" ) ) == 0 );

  rtmpWindow->CreateEx( GetWnd(), NULL, WS_CHILD, 0,
    0, 0, GetClientWidth(), GetClientHeight() - fToolbar.GetWindowHeight() );
  rtmpWindow->Show();
  rtmpWindow->SetTimebase( iTimebase );
  fRtmpWindowArry.push_back( uPeerUUID );

  DWORD dwCxClient = GetClientWidth();
  DWORD dwCyClient = GetClientHeight();
  SendMessage( WM_SIZE, 0, MAKELPARAM( dwCxClient, dwCyClient ) );

  // begin encoding
  if ( !fEncoderThread )
  {
    fEncoderThread = (HANDLE)_beginthreadex( NULL, 0, EncoderThread, rtmpWindow, 0, NULL );
  }
  return TRUE;
}

LRESULT KMainWindow::OnContextMenu( UINT uMsg, WPARAM wParam, LPARAM lParam )
{
  //   POINT pt;
//   HMENU hSubMenu = GetSubMenu( hMenu, 0 );
//   pt.x = LOWORD( lParam );
//   pt.y = HIWORD( lParam );
//   TrackPopupMenu( hSubMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hWnd, NULL );
  return FALSE;
}

LRESULT KMainWindow::OnCommand( UINT uMsg, WPARAM wParam, LPARAM lParam )
{
  switch ( LOWORD(wParam) )
  {
    case IDB_TOOLBAR_VOICE:
    {
      if ( fToolbar.GetButtonImageIndex( IDB_TOOLBAR_VOICE ) == 0 )
      {
        fToolbar.SetButtonImage( IDB_TOOLBAR_VOICE, 1 );
        fToolbar.SetButtonText( IDB_TOOLBAR_VOICE, _T( "静音" ) );
      }
      else
      {
        fToolbar.SetButtonImage( IDB_TOOLBAR_VOICE, 0 );
        fToolbar.SetButtonText( IDB_TOOLBAR_VOICE, _T( "解除静音" ) );
      }
      return TRUE;
    }
    case IDB_TOOLBAR_VIEW:
    {
      if ( fToolbar.GetButtonImageIndex( IDB_TOOLBAR_VIEW ) == 0 )
      {
        fToolbar.SetButtonImage( IDB_TOOLBAR_VIEW, 1 );
        fToolbar.SetButtonText( IDB_TOOLBAR_VIEW, _T("停止视频") );
      }
      else
      {
        fToolbar.SetButtonImage( IDB_TOOLBAR_VIEW, 0 );
        fToolbar.SetButtonText( IDB_TOOLBAR_VIEW, _T( "开启视频" ) );
      }
      return TRUE;
    }
    case IDB_TOOLBAR_SHARE:
      return TRUE;
    case IDB_TOOLBAR_INVITE:
      return TRUE;
    case IDB_TOOLBAR_MANAGE:
      return TRUE;
    case IDB_TOOLBAR_CHATON:
      return TRUE;
    case IDB_TOOLBAR_EMOJI:
      return TRUE;
    case IDB_TOOLBAR_DOCUMENT:
      return TRUE;
    case IDB_TOOLBAR_SETTING:
      return TRUE;
    default:
      break;
  }
  return FALSE;
}

LRESULT KMainWindow::OnPaint( UINT uMsg, WPARAM wParam, LPARAM lParam )
{
  PAINTSTRUCT ps;
  BeginPaint( GetWnd(), &ps );
  EndPaint( GetWnd(), &ps );
  return TRUE;
}

LRESULT KMainWindow::OnDestroy( UINT uMsg, WPARAM wParam, LPARAM lParam )
{
  /*
   * stop encoder thread
   */
  if ( fEncoderThread )
    CloseHandle( fEncoderThread );

  PostQuitMessage( 0 );
  return TRUE;
}

LRESULT KMainWindow::OnNotify( UINT uMsg, WPARAM wParam, LPARAM lParam )
{
  LPNMTOOLBAR lpnmTB = (LPNMTOOLBAR)lParam;
  LPNMMOUSE lpnmMouse = (LPNMMOUSE)lParam;
  TCHAR szBuffer[128];
  switch ( ((LPNMMOUSE)lParam)->hdr.code )
  {
    case NM_CLICK:
    {
      if (!KUtils::IsCommandID(lpnmTB->iItem ))
        break;

      UINT64 uPeerUUID = KUtils::GetUUID( lpnmTB->iItem );
      if ( uPeerUUID == GetUUID() ) break; // 不能跟自己视频
       if ( DialogBoxParam( KUtils::GetInstance(), MAKEINTRESOURCE( IDD_DIALOG_CONFIRM ), GetWnd(), 
         ConfirmDlgProc, (LPARAM)uPeerUUID ) == IDOK )
      {
         KUtils::GetUUIDStr( szBuffer, lpnmTB->iItem );
        KUtils::SubmitWork( EV_WRITE, PacketUtils::build_connect_packet( lstrlen( szBuffer ),
          GetUUIDStr(), KUtils::ToString( uPeerUUID ).c_str() ) );
      }
      break;
    }
    case TBN_HOTITEMCHANGE:
    {
      break;
    }
    case TBN_DROPDOWN:
    {
      RECT rc;
      ::SendMessage( lpnmTB->hdr.hwndFrom, TB_GETRECT, (WPARAM)lpnmTB->iItem, (LPARAM)&rc );
      ::MapWindowPoints( lpnmTB->hdr.hwndFrom, HWND_DESKTOP, (LPPOINT)&rc, 2 );
      TPMPARAMS tpm;
      tpm.cbSize = sizeof( TPMPARAMS );
      tpm.rcExclude = rc;

      HMENU hPopupMenu = CreatePopupMenu();
      switch ( lpnmTB->iItem )
      {
        case IDB_TOOLBAR_VOICE:
        {
          AppendMenu( hPopupMenu, MF_STRING, 0, _T( "音频选项" ) );
          break;
        }
        case IDB_TOOLBAR_VIEW:
        {
          AppendMenu( hPopupMenu, MF_STRING, 0, _T( "视频选项" ) );
          break;
        }
        case IDB_TOOLBAR_SHARE:
        {
          AppendMenu( hPopupMenu, MF_STRING, 0, _T( "全体成员可共享" ) );
          AppendMenu( hPopupMenu, MF_STRING, 0, _T( "仅主持人可共享" ) );
          break;
        }
        default:
          break;
      }
      TrackPopupMenuEx( hPopupMenu,
        TPM_BOTTOMALIGN | TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_VERTICAL,
        rc.left, rc.bottom, GetWnd(), &tpm );
      DestroyMenu( hPopupMenu );
      break;
    }
    default:
      break;
  }
  return FALSE;
}

LRESULT KMainWindow::OnClose( UINT uMsg, WPARAM wParam, LPARAM lParam )
{
  if ( DialogBoxParam( KUtils::GetInstance(), MAKEINTRESOURCE( IDD_DIALOG_EXIT ),
    GetWnd(), ExitDlgProc, 0 ) == IDOK )
    DestroyWindow(GetWnd());
  return FALSE;
}

LRESULT KMainWindow::OnCustom( UINT uMsg, WPARAM wParam, LPARAM lParam )
{
  switch ( uMsg )
  {
    HANDLE_WM_MSG( WM_ONLINE, wParam, lParam, OnOnline );
    HANDLE_WM_MSG( WM_OFFLINE, wParam, lParam, OnOffline );
    HANDLE_WM_MSG( WM_REQUEST, wParam, lParam, OnRequest );
    HANDLE_WM_MSG( WM_REFUSED, wParam, lParam, OnRefused );
    HANDLE_WM_MSG( WM_ACCEPT, wParam, lParam, OnAccept );
    default:
      break;
  }
  return DefWindowProc( uMsg, wParam, lParam );
}

INT_PTR CALLBACK KMainWindow::ConfirmDlgProc( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam )
{
  TCHAR szBuffer[128];
  HWND hWndParent = ::GetParent( hDlg );
  switch ( message )
  {
    case WM_INITDIALOG:
    {
      UINT64 uPeerUUID = lParam;
      wsprintf( szBuffer, _T("您确定要与用户%I64u进行视频会话?"), uPeerUUID );
      HWND hStaticConfirm = GetDlgItem( hDlg, IDC_STATIC_CONFIRM );
      ::SetWindowText( hStaticConfirm, szBuffer );
      return TRUE;
    }
    case WM_COMMAND:
      switch ( LOWORD( wParam ) )
      {
        case IDOK:
          EndDialog( hDlg, IDOK );
          return TRUE;
        case IDCANCEL:
          EndDialog( hDlg, IDCANCEL );
          return TRUE;
      }
      break;
  }
  return FALSE;
}

INT_PTR CALLBACK KMainWindow::RequestDlgProc( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam )
{
  TCHAR szBuffer[128];
  HWND hWndParent = ::GetParent( hDlg );
  switch ( message )
  {
    case WM_INITDIALOG:
    {
      UINT64 uPeerUUID = lParam;
      wsprintf( szBuffer, _T("用户%I64u想要与您进行视频会话?"), uPeerUUID );
      HWND hStaticConfirm = GetDlgItem( hDlg, IDC_STATIC_REQUEST );
      ::SetWindowText( hStaticConfirm, szBuffer );
      return TRUE;
    }
    case WM_COMMAND:
      switch ( LOWORD( wParam ) )
      {
        case IDOK:
          EndDialog( hDlg, IDOK );
          return TRUE;
        case IDCANCEL:
          EndDialog( hDlg, IDCANCEL );
          return TRUE;
      }
      break;
  }
  return FALSE;
}

INT_PTR CALLBACK KMainWindow::RefusedDlgProc( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam )
{
  TCHAR szBuffer[128];
  HWND hWndParent = ::GetParent( hDlg );
  switch ( message )
  {
    case WM_INITDIALOG:
    {
      UINT64 uPeerUUID = lParam;
      wsprintf( szBuffer, _T( "用户%I64u拒绝了您的请求." ), uPeerUUID );
      HWND hStaticRefused = GetDlgItem( hDlg, IDC_STATIC_REFUSED );
      ::SetWindowText( hStaticRefused, szBuffer );
      return TRUE;
    }
    case WM_COMMAND:
      switch ( LOWORD( wParam ) )
      {
        case IDOK:
          EndDialog( hDlg, IDOK );
          return TRUE;
        case IDCANCEL:
          EndDialog( hDlg, IDCANCEL );
          return TRUE;
      }
      break;
  }
  return FALSE;
}

INT_PTR CALLBACK KMainWindow::ExitDlgProc( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam )
{
  TCHAR szBuffer[128];
  HWND hWndParent = ::GetParent(hDlg);
  switch ( message )
  {
    case WM_INITDIALOG:
    {
      wsprintf( szBuffer, _T( "您确定要结束当前会议吗?" ) );
      HWND hStaticRefused = GetDlgItem( hDlg, IDC_STATIC_EXIT );
      ::SetWindowText( hStaticRefused, szBuffer );
      return TRUE;
    }
    case WM_COMMAND:
      switch ( LOWORD( wParam ) )
      {
        case IDOK:
          EndDialog( hDlg, IDOK );
          return TRUE;
        case IDCANCEL:
          EndDialog( hDlg, IDCANCEL );
          return TRUE;
      }
      break;
  }
  return (INT_PTR)FALSE;
}

INT_PTR CALLBACK KMainWindow::AboutDlgProc( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam )
{
  UNREFERENCED_PARAMETER( lParam );
  switch ( message )
  {
    case WM_INITDIALOG:
      return (INT_PTR)TRUE;

    case WM_COMMAND:
      if ( LOWORD( wParam ) == IDOK || LOWORD( wParam ) == IDCANCEL )
      {
        EndDialog( hDlg, LOWORD( wParam ) );
        return (INT_PTR)TRUE;
      }
      break;
  }
  return (INT_PTR)FALSE;
}

unsigned CALLBACK KMainWindow::EncoderThread( void *arg )
{
  /*
  * begin encoding
  */
  KUtils::Log( LOGDEBUG, "encoder thread is start..." );
#if DEBUG_NO_ENCODING
#if DEBUG_DOUBLE_CHECK
  HANDLE hFile = CreateFile( "enc_str.bin", GENERIC_READ, FILE_SHARE_READ, NULL,
    OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL );
#else
  HANDLE hFile = CreateFile( _T("../../../../0110random/enc_str.bin"), GENERIC_READ, FILE_SHARE_READ, NULL,
    OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL );
#endif
  DWORD dwFileSize, dwFileSizeHigh, dwNumberOfBytesRead;
  dwFileSize = GetFileSize( hFile, &dwFileSizeHigh );
  CHAR *szFileData = (CHAR *)malloc( dwFileSize );
  BOOL bSuccess = ReadFile( hFile, szFileData, dwFileSize, &dwNumberOfBytesRead, 0 );
  CloseHandle( hFile );

  CHAR *p = szFileData, *pL = NULL;
  size_t numBytes = 0;
  KRtmpWindow *win = (KRtmpWindow *)arg;
  TCHAR szUUIDStr[64];
  wsprintf( szUUIDStr, _T("%I64u"), KUtils::GetMainWindow()->GetUUID() );
  for ( DWORD k = 0; k < dwFileSize; ++k )
  {
    if ( (*p == 1 && k >= 2) && (!*(p - 1) && !*(p - 2)) )
    {
      if ( pL )
      {
        size_t frameSize = 0;
        if ( !*(p - 3) )
          frameSize = p - pL - 3;
        else
          frameSize = p - pL - 2;
        numBytes += frameSize;
        size_t numPack = NUM_PACK( frameSize );
        uint64_t timestamp = get_timestamp_ms();
        for ( size_t i = 0; i < numPack; ++i )
        {
#if DEBUG_SEND_TO_MYSELF
          win->PriQueue().push( PacketUtils::new_push_packet(
            (uint32_t)frameSize, i != numPack - 1,
            (uint32_t)i * MAX_BODY_SIZE,
            timestamp,
            g_app,
            (char *)pL + i * MAX_BODY_SIZE ) );
#else
          KUtils::SubmitWork( EV_WRITE, PacketUtils::push_packet(
            (uint32_t)frameSize, i != numPack - 1,
            (uint32_t)i * MAX_BODY_SIZE,
            timestamp,
            szUUIDStr,
            (char *)pL + i * MAX_BODY_SIZE ) );
#endif
        }
        msleep( g_timebase );
      }
      pL = p + 1;
    }
    p++;
  }
#if DEBUG_SEND_TO_MYSELF
  win->PriQueue().push( PacketUtils::new_fin_packet( get_timestamp_ms(), g_app ) );
#else
  KUtils::SubmitWork( EV_WRITE, PacketUtils::fin_packet( get_timestamp_ms(), szUUIDStr ) );
#endif

  free( szFileData );
#else
  int argc = 5;
  const char *argv[5] = {
    "",
  "-c",
  "../../../../0110random/BasketballDrill.cfg",
  "-c",
  "../../../../0110random/encoder_randomaccess_vtm.cfg" };

#if ENABLE_SIMD_OPT
  std::string SIMD;
  df::program_options_lite::Options opts;
  opts.addOptions()
    ("SIMD", SIMD, string( "" ), "")
    ("c", df::program_options_lite::parseConfigFile, "");
  df::program_options_lite::SilentReporter err;
  df::program_options_lite::scanArgv( opts, argc, (const char**)argv, err );
  RTMP_Log( RTMP_LOGDEBUG, "[SIMD=%s] ", read_x86_extension( SIMD ) );
#endif
#if ENABLE_TRACING
  RTMP_Log( RTMP_LOGDEBUG, "[ENABLE_TRACING] " );
#endif
#if ENABLE_SPLIT_PARALLELISM
  RTMP_Log( RTMP_LOGDEBUG, "[SPLIT_PARALLEL (%d jobs)]", PARL_SPLIT_MAX_NUM_JOBS );
#endif
#if ENABLE_SPLIT_PARALLELISM
  const char* waitPolicy = getenv( "OMP_WAIT_POLICY" );
  const char* maxThLim = getenv( "OMP_THREAD_LIMIT" );
  RTMP_Log( RTMP_LOGDEBUG, waitPolicy ? "[OMP: WAIT_POLICY=%s," : "[OMP: WAIT_POLICY=,", waitPolicy );
  RTMP_Log( RTMP_LOGDEBUG, maxThLim ? "THREAD_LIMIT=%s" : "THREAD_LIMIT=", maxThLim );
  RTMP_Log( RTMP_LOGDEBUG, "]" );
#endif
  RTMP_Log( RTMP_LOGDEBUG, "\n" );

#if JVET_N0278_FIXES
  std::fstream bitstream;
  EncLibCommon encLibCommon;

  std::vector<EncApp*> pcEncApp( 1 );
  bool resized = false;
  int layerIdx = 0;

  initROM();
  TComHash::initBlockSizeToIndex();

  char** layerArgv = new char*[argc];

  do
  {
    pcEncApp[layerIdx] = new EncApp( bitstream, &encLibCommon );
    // create application encoder class per layer
    pcEncApp[layerIdx]->create();

    // parse configuration per layer
    try
    {
      int j = 0;
      for ( int i = 0; i < argc; i++ )
      {
        if ( argv[i][0] == '-' && argv[i][1] == 'l' )
        {
          if ( argv[i][2] == std::to_string( layerIdx ).c_str()[0] )
          {
            layerArgv[j] = (char *)argv[i + 1];
            layerArgv[j + 1] = (char *)argv[i + 2];
            j += 2;
          }
          i += 2;
        }
        else
        {
          layerArgv[j] = (char *)argv[i];
          j++;
        }
      }

      if ( !pcEncApp[layerIdx]->parseCfg( j, layerArgv ) )
      {
        pcEncApp[layerIdx]->destroy();
        return 0;
      }
    }
    catch ( df::program_options_lite::ParseFailure &e )
    {
      std::cerr << "Error parsing option \"" << e.arg << "\" with argument \"" << e.val << "\"." << std::endl;
      return 0;
    }

    int layerId = layerIdx; //VS: layerIdx should be converted to layerId after VPS is implemented
    pcEncApp[layerIdx]->createLib( layerId );

    if ( !resized )
    {
      pcEncApp.resize( pcEncApp[layerIdx]->getMaxLayers() );
      resized = true;
    }

    layerIdx++;
  } while ( layerIdx < pcEncApp.size() );

  delete[] layerArgv;
#else
  EncApp* pcEncApp = new EncApp;
  // create application encoder class
  pcEncApp->create();

  // parse configuration
  try
  {
    if ( !pcEncApp->parseCfg( argc, argv ) )
    {
      pcEncApp->destroy();
      return 1;
    }
  }
  catch ( df::program_options_lite::ParseFailure &e )
  {
    std::cerr << "Error parsing option \"" << e.arg << "\" with argument \"" << e.val << "\"." << std::endl;
    return 1;
  }
#endif

#if PRINT_MACRO_VALUES
  //printMacroSettings();
#endif

  // starting time
  auto startTime = std::chrono::steady_clock::now();
  std::time_t startTime2 = std::chrono::system_clock::to_time_t( std::chrono::system_clock::now() );
  RTMP_Log( RTMP_LOGDEBUG, " started @ %s", std::ctime( &startTime2 ) );
  clock_t startClock = clock();

#if JVET_N0278_FIXES
  // call encoding function per layer
  bool eos = false;

  while ( !eos )
  {
    // read GOP
    bool keepLoop = true;
    while ( keepLoop )
    {
      for ( auto & encApp : pcEncApp )
      {
#ifndef _DEBUG
        try
        {
#endif
          keepLoop = encApp->encodePrep( eos );
#ifndef _DEBUG
        }
        catch ( Exception &e )
        {
          std::cerr << e.what() << std::endl;
          return EXIT_FAILURE;
        }
        catch ( const std::bad_alloc &e )
        {
          std::cout << "Memory allocation failed: " << e.what() << std::endl;
          return EXIT_FAILURE;
        }
#endif
      }
    }

    // encode GOP
    keepLoop = true;
    while ( keepLoop )
    {
      for ( auto & encApp : pcEncApp )
      {
#ifndef _DEBUG
        try
        {
#endif
          keepLoop = encApp->encode();
#ifndef _DEBUG
        }
        catch ( Exception &e )
        {
          std::cerr << e.what() << std::endl;
          return EXIT_FAILURE;
        }
        catch ( const std::bad_alloc &e )
        {
          std::cout << "Memory allocation failed: " << e.what() << std::endl;
          return EXIT_FAILURE;
        }
#endif
      }
    }
  }
#else
  // call encoding function
#ifndef _DEBUG
  try
  {
#endif
    pcEncApp->encode();
#ifndef _DEBUG
  }
  catch ( Exception &e )
  {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }
  catch ( const std::bad_alloc &e )
  {
    std::cout << "Memory allocation failed: " << e.what() << std::endl;
    return EXIT_FAILURE;
  }
#endif
#endif
  // ending time
  clock_t endClock = clock();
  auto endTime = std::chrono::steady_clock::now();
  std::time_t endTime2 = std::chrono::system_clock::to_time_t( std::chrono::system_clock::now() );
#if JVET_O0756_CALCULATE_HDRMETRICS
#if JVET_N0278_FIXES
  auto metricTime = pcEncApp[0]->getMetricTime();

  for ( int layerIdx = 1; layerIdx < pcEncApp.size(); layerIdx++ )
  {
    metricTime += pcEncApp[layerIdx]->getMetricTime();
  }
#else
  auto metricTime = pcEncApp->getMetricTime();
#endif
  auto totalTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
  auto encTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime - metricTime).count();
  auto metricTimeuser = std::chrono::duration_cast<std::chrono::milliseconds>(metricTime).count();
#else
  auto encTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
#endif

#if JVET_N0278_FIXES
  for ( auto & encApp : pcEncApp )
  {
    encApp->destroyLib();

    // destroy application encoder class per layer
    encApp->destroy();

    delete encApp;
  }

  // destroy ROM
  destroyROM();

  pcEncApp.clear();
#else
  // destroy application encoder class
  pcEncApp->destroy();

  delete pcEncApp;
#endif

  RTMP_Log( RTMP_LOGDEBUG, "\n finished @ %s", std::ctime( &endTime2 ) );

#if JVET_O0756_CALCULATE_HDRMETRICS
  RTMP_Log( RTMP_LOGDEBUG, " Encoding Time (Total Time): %12.3f ( %12.3f ) sec. [user] %12.3f ( %12.3f ) sec. [elapsed]\n",
    ((endClock - startClock) * 1.0 / CLOCKS_PER_SEC) - (metricTimeuser / 1000.0),
    (endClock - startClock) * 1.0 / CLOCKS_PER_SEC,
    encTime / 1000.0,
    totalTime / 1000.0 );
#else
  RTMP_Log( RTMP_LOGDEBUG, " Total Time: %12.3f sec. [user] %12.3f sec. [elapsed]\n",
    (endClock - startClock) * 1.0 / CLOCKS_PER_SEC,
    encTime / 1000.0 );
#endif
#endif
  return 0;
}

#if DEBUG_LOCAL_VIDEO_TEST
HBITMAP KMainWindow::NextFrame( HDC hdc, HANDLE hFile, DWORD cxOrigPic, DWORD cyOrigPic )
{
  if ( hFile == INVALID_HANDLE_VALUE )
    return NULL;

  DWORD dwNumberOfBytesRead = 0;
  DWORD dwReadSize = cxOrigPic * cyOrigPic * 3 / 2;

  BYTE *pBuffer = (BYTE *)malloc( dwReadSize );

  BOOL bSuccess = ReadFile( hFile, pBuffer, dwReadSize, &dwNumberOfBytesRead, 0 );
  if ( dwReadSize != dwNumberOfBytesRead )
  {
    CloseHandle( hFile );
    return NULL;
  }

  return KUtils::YUVtoBitmap( hdc, pBuffer, cxOrigPic, cyOrigPic );
}

#endif