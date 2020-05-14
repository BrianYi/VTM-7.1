#include "KHeader.h"
#include "event2/event.h"
#include "event2/bufferevent.h"
#include "event2/thread.h"

namespace K
{
  HINSTANCE KUtils::sInst;
  TCHAR KUtils::sResPath[MAX_PATH];
  TCHAR KUtils::sProgName[MAX_PATH];
  FILE *KUtils::sLogFile;
  LogLevel KUtils::sLogLevel;
  UINT64 KUtils::sUUID64Counter;
  //UINT32 KUtils::sUUID32Counter;
  INT32 KUtils::sCommandIDCounter = KUtils::GetCommandIDBase();
  KMutex KUtils::sUtilsMutex;
  KCond KUtils::sDispatcherCond;
  KMutex KUtils::sDispatcherMutex;
  struct event_base* KUtils::sEventBase;
  struct bufferevent* KUtils::sBufferevent;
  PTP_POOL KUtils::sPTPPool;
  PTP_CALLBACK_ENVIRON KUtils::sPTPCallEnv;
  HANDLE KUtils::sDispatcherThread;
  PriorityQueue KUtils::sOutputQueue;
  KObject *KUtils::sMainWindow;
  std::unordered_map<UINT64, KObject*> KUtils::sHashUUIDtoWin;
  std::unordered_map<UINT64, INT32> KUtils::sUUIDtoCommandID;
  //  std::unordered_set<std::string> KUtils::sClassSet;

  BOOL KUtils::Initialize( HINSTANCE inst, const TCHAR* szResPath, const TCHAR* szProgName )
  {
#if _WIN32
    WSADATA wsaData;
    WSAStartup( MAKEWORD( 2, 2 ), &wsaData );
#endif

    KUtils::SetInstance( inst );
    KUtils::SetResourcePath( szResPath );
    KUtils::SetProgramName( szProgName );

    /*
     * Start log thread
     */
    FILE* dumpfile = NULL;
    dumpfile = _fsopen( "RTMPlayer.dump", "a+", _SH_DENYNO );
    KUtils::LogSetFile( dumpfile );
    KUtils::LogSetLevel( LOGALL );
    KUtils::LogStart();
    
    SYSTEMTIME tm;
    GetSystemTime( &tm );
    KUtils::Log( LOGDEBUG, "==============================" );
    KUtils::Log( LOGDEBUG, "log file:\tRTMPlayer.dump" );
    KUtils::Log( LOGDEBUG, "log timestamp:\t%lld", KUtils::GetTimestampMS() );
    KUtils::Log( LOGDEBUG, "log date:\t%hu-%hu-%hu %hu:%hu:%hu",
      tm.wYear,
      tm.wMonth,
      tm.wDay,
      tm.wHour, tm.wMinute, tm.wSecond );
    KUtils::Log( LOGDEBUG, "==============================" );


    /*
     * Create Thread Pool
     */
    SYSTEM_INFO sysinfo;
    GetSystemInfo( &sysinfo );
    int numOfProcessors = sysinfo.dwNumberOfProcessors;
    sPTPPool = CreateThreadpool( NULL );
    SetThreadpoolThreadMaximum( sPTPPool, 1/*numOfProcessors*/ );
    SetThreadpoolThreadMinimum( sPTPPool, 1 );
    
    /*
     * start event thread
     * use _beginthread, it will automatically stopping when exit
     */
    sDispatcherThread = (HANDLE)_beginthreadex( NULL, 0, DispatcherThread, 0/*this*/, 0, NULL );
    return TRUE;
  }

  BOOL KUtils::CleanUp()
  {
    /*
   * stop dispatcher thread
   */
    event_base_loopbreak( sEventBase );

    /*
     * stop thread pool
     */
    if ( sPTPPool )
      CloseThreadpool( sPTPPool );

    /*
   * destroy child window(include threads in child window )
   */
//     for ( auto it = g_umHashToSession.begin();
//       it != g_umHashToSession.end();
//       ++it )
//     {
//       Session *s = it->second;
//       delete s;
//     }
//     g_umHashToSession.clear();
// 
//     for ( auto it = g_umHashToWin.begin();
//       it != g_umHashToWin.end();
//       ++it )
//     {
//       KRtmpWindow *w = it->second;
//       delete w;
//     }
//     g_umHashToWin.clear();

    /*
     * stop log thread
     */
    KUtils::LogStop();

#if _WIN32
    WSACleanup();
#endif

    return TRUE;
  }

//   BOOL KUtils::SetClass( const TCHAR* lpszClassName )
//   {
//     if ( sClassSet.count( lpszClassName ) )
//       return FALSE;
//     sClassSet.insert( lpszClassName );
//     return TRUE;
//   }
// 
//   BOOL KUtils::CountClass( const TCHAR* lpszClassName )
//   {
//     return sClassSet.count( lpszClassName );
//   }

  HBITMAP KUtils::LoadImage( const TCHAR* szFileName )
  {
    TCHAR szBuffer[MAX_PATH];
    wsprintf( szBuffer, _T("%s/%s"), KUtils::GetResourcePath(), szFileName );
    HANDLE hFile = ::CreateFile( szBuffer, GENERIC_READ, FILE_SHARE_READ, NULL,
      OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
    DWORD dwFileSize = ::GetFileSize( hFile, NULL );

    int x, y, n;
    BYTE *pbyData = (BYTE*)malloc( sizeof( BYTE ) * dwFileSize );
    DWORD dwNumOfRead = 0;
    ReadFile( hFile, pbyData, dwFileSize, &dwNumOfRead, NULL );
    assert( dwNumOfRead == dwFileSize );
    ::CloseHandle( hFile );

    LPBYTE pImage = stbi_load_from_memory( pbyData, dwFileSize, &x, &y, &n, 4 );
    free( pbyData );
    assert( pImage );
    BITMAPINFO bmi;
    ::ZeroMemory( &bmi, sizeof( BITMAPINFO ) );
    bmi.bmiHeader.biSize = sizeof( BITMAPINFOHEADER );
    bmi.bmiHeader.biWidth = x;
    bmi.bmiHeader.biHeight = -y;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    bmi.bmiHeader.biSizeImage = x * y * 4;

    LPBYTE pbyDest = nullptr;
    HBITMAP hBitmap = ::CreateDIBSection( nullptr, &bmi, DIB_RGB_COLORS, 
      (void**)&pbyDest, nullptr, 0 );
    assert( hBitmap );

    for ( int i = 0; i < x * y; i++ )
    {
      pbyDest[i * 4 + 3] = pImage[i * 4 + 3];
      if ( pbyDest[i * 4 + 3] < 255 )
      {
        pbyDest[i * 4] = (BYTE)(DWORD( pImage[i * 4 + 2] )*pImage[i * 4 + 3] / 255);
        pbyDest[i * 4 + 1] = (BYTE)(DWORD( pImage[i * 4 + 1] )*pImage[i * 4 + 3] / 255);
        pbyDest[i * 4 + 2] = (BYTE)(DWORD( pImage[i * 4] )*pImage[i * 4 + 3] / 255);
      }
      else
      {
        pbyDest[i * 4] = pImage[i * 4 + 2];
        pbyDest[i * 4 + 1] = pImage[i * 4 + 1];
        pbyDest[i * 4 + 2] = pImage[i * 4];
      }
    }

    stbi_image_free( pImage );

    return hBitmap;
  }

  BOOL KUtils::SetWindow( KObject *obj )
  {
    KMutexLocker locker( &sUtilsMutex );
    assert( sHashUUIDtoWin.count( obj->GetUUID() ) == 0 );
    sHashUUIDtoWin[obj->GetUUID()] = obj;
    return TRUE;
  }

  KObject * KUtils::GetWindow( const UINT64& uUUID )
  {
    KMutexLocker locker( &sUtilsMutex );
    if ( sHashUUIDtoWin.count( uUUID ) )
      return sHashUUIDtoWin[uUUID];
    return nullptr;
  }

  UINT64 KUtils::GetTimestampMS()
  {
    FILETIME ft;
    GetSystemTimeAsFileTime( &ft );
    return  ((UINT64)ft.dwLowDateTime + ((UINT64)ft.dwHighDateTime << 32)) / 10000;
  }

  UINT64 KUtils::GetTimestampS()
  {
    return GetTimestampMS() / 1000;
  }

  UINT64 KUtils::GenUUID()
  {
    KMutexLocker locker( &sUtilsMutex );
    UINT64 currentMS = GetTimestampMS();
    if ( currentMS * 1000 > sUUID64Counter )
      sUUID64Counter = currentMS * 1000;
    else
      sUUID64Counter++;
    return sUUID64Counter;
  }

  INT32 KUtils::GetCommandID( const UINT64& uUUID )
  {
    KMutexLocker locker( &sUtilsMutex );
    if ( !sUUIDtoCommandID.count( uUUID ) )
    {
      sUUIDtoCommandID[uUUID] = sCommandIDCounter;
      sCommandIDCounter++;
    }
    return sUUIDtoCommandID[uUUID];
  }

  UINT64 KUtils::GetUUID( const INT32& iCommandID )
  {
    KMutexLocker locker( &sUtilsMutex );
    for(auto it= sUUIDtoCommandID.begin(); it != sUUIDtoCommandID.end(); ++it) 
    {
      if ( it->second == iCommandID )
        return it->first;
    }
    return 0;
  }

  BOOL KUtils::GetUUIDStr( TCHAR* szBuffer, const INT32& iCommandID )
  {
    wsprintf( szBuffer, _T( "%I64u" ), GetUUID( iCommandID ) );
    return TRUE;
  }

//   UINT32 KUtils::GenUUID()
//   {
//     KMutexLocker locker( &sUtilsMutex );
//     UINT64 currentMS = GetTimestampMS();
//     if ( currentMS * 1000 > sUUID32Counter )
//       sUUID32Counter = currentMS * 1000;
//     else
//       sUUID32Counter++;
//     return sUUID32Counter;
//   }

  std::string KUtils::ToString( const UINT64& uINT64 )
  {
    return std::to_string( uINT64 );
  }

  std::string KUtils::ToString( const UINT32& uINT32 )
  {
    return std::to_string( uINT32 );
  }

  INT64 KUtils::ToInt64( const KString& kStr )
  {
    return _tstoll( kStr );
  }

  INT32 KUtils::ToInt32( const KString& kStr )
  {
    return _tstoi( kStr );
  }

  HBITMAP KUtils::YUVtoBitmap( HDC hdc, BYTE *pYUVData, DWORD cxOrigPic, DWORD cyOrigPic )
  {
    BYTE *pY = pYUVData;
    BYTE *pCb = pYUVData + cxOrigPic * cyOrigPic;
    BYTE *pCr = pCb + cxOrigPic * cyOrigPic / 4;

    DWORD *pRGBArry = (DWORD *)malloc( cxOrigPic * cyOrigPic * sizeof DWORD );
    ZeroMemory( pRGBArry, cxOrigPic * cyOrigPic * sizeof DWORD );

    DWORD x, y;
    BYTE Y, Cb, Cr;
    int r, g, b;
    for ( y = 0; y < cyOrigPic; ++y )
    {
      for ( x = 0; x < cxOrigPic; ++x )
      {
        // get yuv component
        Y = pY[y * cxOrigPic + x];
        Cb = pCb[(y / 2) * (cxOrigPic / 2) + (x / 2)];
        Cr = pCr[(y / 2) * (cxOrigPic / 2) + (x / 2)];
        // r,g,b
        r = int( Y + 1.402 * (Cr - 128) );
        g = int( Y - (0.344 * (Cb - 128)) - (0.714 * (Cr - 128)) );
        b = int( Y + 1.772 * (Cb - 128) );
        // clamp
        r = min( max( 0, r ), 255 );
        g = min( max( 0, g ), 255 );
        b = min( max( 0, b ), 255 );

        pRGBArry[y * cxOrigPic + x] = ((r << 16) | (g << 8) | b);
      }
    }

    BITMAPINFO bmi;
    ZeroMemory( &bmi, sizeof BITMAPINFO );
    BITMAPINFOHEADER& bmih = bmi.bmiHeader;
    bmih.biSize = sizeof BITMAPINFOHEADER;
    bmih.biWidth = cxOrigPic;
    bmih.biHeight = cyOrigPic;
    bmih.biHeight = -bmih.biHeight;
    bmih.biPlanes = 1;
    bmih.biBitCount = 32;
    bmih.biCompression = BI_RGB;

    HBITMAP hBitmap = CreateCompatibleBitmap( hdc, cxOrigPic, cyOrigPic );
    HDC hdcMem = CreateCompatibleDC( hdc );
    SelectObject( hdcMem, hBitmap );
    SetDIBits( hdcMem, hBitmap, 0, cyOrigPic, pRGBArry, &bmi, 0 );
    //free( pBuffer );
    free( pRGBArry );
    DeleteDC( hdcMem );
    return hBitmap;
  }

  std::vector<std::string> KUtils::SplitString( std::string sourStr, std::string delimiter )
  {
    std::vector<std::string> strArry;
    size_t pos = 0;
    std::string token;
    while ( (pos = sourStr.find( delimiter )) != std::string::npos )
    {
      token = sourStr.substr( 0, pos );
      strArry.push_back( token );
      sourStr.erase( 0, pos + delimiter.length() );
    }
    return strArry;
  }

  BOOL KUtils::LogSetFile( FILE *file )
  {
    sLogFile = file;
    RTMP_LogSetOutput( sLogFile );
    return TRUE;
  }

  BOOL KUtils::LogSetLevel( LogLevel logLevel )
  {
    sLogLevel = logLevel;
    RTMP_LogSetLevel( (RTMP_LogLevel)logLevel );
    return TRUE;
  }

  BOOL KUtils::LogStart()
  {
    RTMP_LogThreadStart();
    return TRUE;
  }

  BOOL KUtils::LogStop()
  {
    RTMP_LogThreadStop();
    return TRUE;
  }

  BOOL KUtils::Log( LogLevel logLevel, const char *format, ... )
  {
    char str[2048];
    va_list args;
    va_start( args, format );

    vsnprintf( str, 2047, format, args );

    RTMP_Log( (RTMP_LogLevel)logLevel, str );
    va_end( args );
    return TRUE;
  }

  BOOL KUtils::LogAndPrint( LogLevel logLevel, const char *format, ... )
  {
    RTMP_LogAndPrintf( (RTMP_LogLevel)logLevel, format );
    return TRUE;
  }
  
  BOOL KUtils::WaitDispatcherStartFinished()
  {
     KMutexLocker locker( &sDispatcherMutex );
     KUtils::sDispatcherCond.Wait( &sDispatcherMutex );
     return TRUE;
  }

  /*
   * below is a piece of shit
   */
  unsigned CALLBACK KUtils::DispatcherThread( void* arg )
  {
    //KBase *pWindow = (KBase *)arg;
    /*
     * Libevent initialization
     */
    struct sockaddr_in sin;
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = htonl( SERVER_IP );
    sin.sin_port = htons( SERVER_PORT );

    evutil_socket_t fdClient = ::socket( AF_INET, SOCK_STREAM, 0 );
    int ret = connect( fdClient, (struct sockaddr *)&sin, sizeof( sin ) );
    if ( ret < 0 )
    {
      ::MessageBox( NULL, TEXT( "Cannot connect to server!" ), KUtils::GetProgramName(), 
        MB_ICONERROR );
      {
        KMutexLocker locker( &KUtils::sDispatcherMutex );
        KUtils::sDispatcherCond.Signal();
      }
      return 0;
    }
    // use libevent in multi-thread environment
    evthread_use_windows_threads();

    evutil_make_socket_nonblocking( fdClient );
    //  int32_t slen = sizeof( sin );
    //   getsockname( fdClient, (struct sockaddr *)&sin, &slen );
    //   fApp = ::inet_ntoa( sin.sin_addr ) +
    //     std::string( ":" ) +
    //     std::to_string( ntohs( sin.sin_port ) );
//     KString str;
//     wsprintf( str, "%s", pWindow->GetWindowText() );
//     pWindow->SetWindowText( str );

    sEventBase = event_base_new();
    sBufferevent = bufferevent_socket_new( sEventBase,
      fdClient,
      BEV_OPT_CLOSE_ON_FREE );
    bufferevent_setcb( sBufferevent, ReadCallback, WriteCallback, EventCallback, NULL );
    bufferevent_enable( sBufferevent, EV_READ | EV_WRITE | EV_PERSIST );

    {
      KMutexLocker locker( &KUtils::sDispatcherMutex );
      KUtils::sDispatcherCond.Signal();
    }
    event_base_dispatch( sEventBase );

    /*
     * cleanup libevent objects
     */
    if ( sBufferevent )
      bufferevent_free( sBufferevent );
    if ( sEventBase )
      event_base_free( sEventBase );
    return 0;
  }

  void KUtils::ReadCallback( struct bufferevent *bev, void *arg )
  {
    char buf[MAX_PACKET_SIZE];
    size_t readSize = bufferevent_read( bev, buf, MAX_PACKET_SIZE );
    if ( readSize == 0 )
      return;
    else
    {
      if ( readSize != MAX_PACKET_SIZE )
        return;
    }

    SubmitWork( EV_READ, Packet( buf ) );
  }

  void KUtils::WriteCallback( struct bufferevent *bev, void *arg )
  {
    int ret = 0;
    while ( !sOutputQueue.empty() )
    {
      Packet* ptrPacket = sOutputQueue.top();
      sOutputQueue.pop();
      PACKET rawNetPacket = ptrPacket->raw_net_packet();
      ret = bufferevent_write( bev, (void *)&rawNetPacket, MAX_PACKET_SIZE );
      if ( ret < 0 )
      {
        sOutputQueue.push( ptrPacket );
        break;
      }
      delete ptrPacket;
    }
  }

  void KUtils::EventCallback( struct bufferevent *bev, short error, void *arg )
  {
    //KMainWindow *pWindow = (KMainWindow *)arg;
    if ( error & BEV_EVENT_EOF )
    {
      MessageBox( NULL, TEXT( "connection has closed!" ),
        KUtils::GetProgramName(), MB_ICONINFORMATION );
    }
    else if ( error & BEV_EVENT_ERROR )
    {
      MessageBox( NULL, TEXT( "error has ocurred!" ),
        KUtils::GetProgramName(), MB_ICONERROR );
    }
    else if ( error & BEV_EVENT_TIMEOUT )
    {
      MessageBox( NULL, TEXT( "timeout!" ),
        KUtils::GetProgramName(), MB_ICONINFORMATION );
    }
  }

  void KUtils::SubmitWork( int flags, const Packet& packet )
  {
    if ( !sEventBase || !sBufferevent )
      return;

    if ( flags & EV_READ )
    {
      PARAMS *ptrParams = new PARAMS;
      ptrParams->flags = flags;
      ptrParams->ptrPacket = PacketUtils::new_packet( packet );
      PTP_WORK work = CreateThreadpoolWork( ReceiveCallback, ptrParams, NULL );
      SubmitThreadpoolWork( work );
    }
    else if ( flags & EV_WRITE )
    {
      sOutputQueue.push( PacketUtils::new_packet( packet ) );
      bufferevent_enable( sBufferevent, EV_WRITE );
    }
  }

  void CALLBACK KUtils::ReceiveCallback(
      PTP_CALLBACK_INSTANCE Instance,
      PVOID                 params,
      PTP_WORK              Work
    )
  {
    PARAMS *ptrParams = (PARAMS *)params;
    Packet *ptrPacket = ptrParams->ptrPacket;
    int flags = ptrParams->flags;
    UINT64 uPeerUUID = std::stoull(ptrPacket->app());
    TCHAR szBuffer[MAX_PACKET_SIZE];
    if ( flags & EV_READ )
    {
#if 1
      uint64_t recvTimestamp = get_timestamp_ms();
      KUtils::Log( LOGDEBUG, "recv packet(%d) from %s, %dB:[%u,%u-%u], packet timestamp=%llu, recv timestamp=%llu, R-P=%llu",
        ptrPacket->type(),
        ptrPacket->app().c_str(),
        MAX_PACKET_SIZE,
        ptrPacket->size(),
        ptrPacket->seq(),
        ptrPacket->seq() + ptrPacket->body_size(),
        ptrPacket->timestamp(),
        recvTimestamp,
        recvTimestamp - ptrPacket->timestamp() );
#endif
#if KEEP_TRACK_PACKET_RCV_HEX
      RTMP_LogHexStr( RTMP_LOGDEBUG, (uint8_t *)&ptrPkt->raw_net_packet(), ptrPkt->packet_size() );
#endif // _DEBUG

      switch ( ptrPacket->type() )
      {
        case Push:
        case Fin:
        {
          if ( sHashUUIDtoWin.count( uPeerUUID ) )
          {
            KRtmpWindow *rtmpWindow = (KRtmpWindow *)sHashUUIDtoWin[uPeerUUID];
            PriorityQueue& priQue = rtmpWindow->PriQueue();
            priQue.push( PacketUtils::new_packet( *ptrPacket ) );
          }
          break;
        }
        case Pull:
          break;
        case Ack:
          break;
        case Err:
          break;
        case OnlineSessions:
        {
          memcpy( szBuffer, ptrPacket->body(), ptrPacket->body_size() );
          szBuffer[ptrPacket->body_size()] = 0;
          KUtils::Log( LOGDEBUG, "OnlineSessions from server: %s", szBuffer );
          std::vector<std::string> appArry = SplitString( szBuffer, "\n" );
          for ( int i = 0; i < appArry.size(); ++i )
          {
            // new session
            uPeerUUID = std::stoull( appArry[i] );
            KRtmpWindow *rtmpWindow = new KRtmpWindow(uPeerUUID);
            KUtils::GetMainWindow()->PostMessage( WM_ONLINE, uPeerUUID, 0 );
            
//             Session *s = new Session;
//             s->app = appArry[i];
//             s->hash = stdext::hash_value( s->app );
//             s->menuid = uuid32();
//             s->win = NULL;  // create by MainWindow when receive accept or buildconnect
// 
//             // insert to hash2session
//             assert( g_umHashToSession.count( s->hash ) == 0 );
//             g_umHashToSession.insert( std::make_pair( s->hash, s ) );
// 
//             // notify MainWindow
//             PostMessage( hWndMain, WM_ONLINE, (WPARAM)s, 0 );
          }
          break;
        }
        case NewSession:
        {
          KUtils::Log( LOGDEBUG, "NewSession from %s", ptrPacket->app().c_str() );
          // new session
          KRtmpWindow *rtmpWindow = new KRtmpWindow( uPeerUUID );
          KUtils::GetMainWindow()->SendMessage( WM_ONLINE, uPeerUUID, 0 );

//           KUtils::Log( LOGDEBUG, "NewSession from %s", ptrPacket->app().c_str() );
//           // new session
//           Session *s = new Session;
//           s->app = ptrPacket->app();
//           s->hash = stdext::hash_value( s->app );
//           s->menuid = uuid32();
//           s->win = NULL;  // create by MainWindow when receive accept or buildconnect
// 
//           // insert to hash2session
//           assert( g_umHashToSession.count( s->hash ) == 0 );
//           g_umHashToSession.insert( std::make_pair( s->hash, s ) );
// 
//           // notify MainWindow
//           PostMessage( hWndMain, WM_ONLINE, (WPARAM)s, 0 );
          break;
        }
        case LostSession:
        {
          KUtils::Log( LOGDEBUG, "LostSession from %s", ptrPacket->app().c_str() );
          KUtils::GetMainWindow()->PostMessage( WM_OFFLINE, uPeerUUID, 0 );
//           uint32_t menuid = 0;
//           bool atLeatDeleteOne = false;
//           if ( g_umHashToSession.count( uPeerUUID ) )
//           {
//             Session *s = g_umHashToSession[uPeerUUID];
//             menuid = s->menuid;
//             g_umHashToSession.erase( uPeerUUID );
//             //delete s;
//             atLeatDeleteOne = true;
//           }
//           if ( sHashUUIDtoWin.count( uPeerUUID ) )
//           {
//             KRtmpWindow *w = sHashUUIDtoWin[uPeerUUID];
//             menuid = w->menuid();
//             sHashUUIDtoWin.erase( uPeerUUID );
//             //delete w;
//             atLeatDeleteOne = true;
//           }
//           assert( atLeatDeleteOne );
//           PostMessage( hWndMain, WM_OFFLINE, (WPARAM)menuid, 0 );
          break;
        }
        case BuildConnect:
        {
          KUtils::Log( LOGDEBUG, "BuildConnect from %s", ptrPacket->app().c_str() );
          assert( KUtils::GetWindow( uPeerUUID ) );
          KUtils::GetMainWindow()->PostMessage( WM_REQUEST, (WPARAM)uPeerUUID, 0 );
          break;
        }
        case Accept:
        {
          KUtils::Log( LOGDEBUG, "Accept from %s", ptrPacket->app().c_str() );
          assert( KUtils::GetWindow( uPeerUUID ) );
          KUtils::GetMainWindow()->PostMessage( WM_ACCEPT, (WPARAM)uPeerUUID, ptrPacket->timebase() );

          break;
        }
        case Refuse:
        {
          KUtils::Log( LOGDEBUG, "Refused from %s", ptrPacket->app().c_str() );
          assert( KUtils::GetWindow( uPeerUUID ) );
          KUtils::GetMainWindow()->PostMessage( WM_REFUSED, (WPARAM)uPeerUUID, 0 );
          break;
        }
        // 	case TypeNum:
        // 		break;
        default:

          break;
      }
    }
    delete ptrPacket;
    delete ptrParams;
    return;
  }
};