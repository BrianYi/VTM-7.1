//#include "DetectMemoryLeaking.h"
#include "EncoderLib/EncLibCommon.h"
#include "EncoderApp/EncApp.h"
#include "Utilities/program_options_lite.h"

#include "DecoderApp/DecApp.h"
#include "program_options_lite.h"
#include "PlayerHeader.h"
#include <event2/event.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/util.h>
#include <event2/thread.h>
#include <atomic>
#include "resource.h"
#include "PlayerUtils.h"
#include "Packet.h"
#include "RtmpUtils/Log.h"

#include <windows.h>


#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "event.lib")

LRESULT CALLBACK WndProc( HWND, UINT, WPARAM, LPARAM );
LRESULT CALLBACK ChildWndProc( HWND, UINT, WPARAM, LPARAM );
INT_PTR CALLBACK    About( HWND, UINT, WPARAM, LPARAM );
static TCHAR szAppName[] = TEXT( "RTMPlayer" );
static TCHAR szChildClassName[] = TEXT( "ChildRTMPlayer" );
struct bufferevent *g_bev;
std::string g_app;
int32_t g_timebase = 1000 / 25;
HWND g_hWndMain;
HINSTANCE g_hIns;
BOOL g_bIsEncoding = FALSE;
// TODO: menu to show online user will cause potential problem
HANDLE g_hEncoderThread;
PriorityQueue g_outputQue;

static struct event_base *g_base;


static std::unordered_map<size_t, RtmpWindow*> sUnorderedMapHashParams;

int WINAPI WinMain( _In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd )
{
#ifdef _DEBUG
  //_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif // _DEBUG

  /*
   * Register Main Window Class
   */
  MSG msg;
  WNDCLASSEX wc;
  memset( &wc, 0, sizeof( wc ) );
  wc.cbSize = sizeof( WNDCLASSEX );
  wc.style = CS_HREDRAW | CS_VREDRAW;
  wc.lpfnWndProc = WndProc;
  wc.cbClsExtra = 0;
  wc.cbWndExtra = 0;
  wc.hInstance = hInstance;
  wc.hIcon = LoadIcon( hInstance, MAKEINTRESOURCE( IDI_PLAYER ) );
  wc.hCursor = LoadCursor( NULL, IDC_ARROW );
  wc.hbrBackground = (HBRUSH)GetStockObject( BLACK_BRUSH );
  wc.lpszMenuName = MAKEINTRESOURCE( IDR_MENU );
  wc.lpszClassName = szAppName;
  g_hIns = hInstance;
  if ( !RegisterClassEx( &wc ) )
  {
    MessageBox( NULL, TEXT( "Program requires Windows NT!" ),
      szAppName, MB_ICONERROR );
    return 0;
  }

  /*
   * Register Child Window Class
   */
  wc.lpfnWndProc = ChildWndProc;
  wc.hIcon = NULL;
  wc.lpszMenuName = NULL;
  wc.lpszClassName = szChildClassName;
  wc.hbrBackground = (HBRUSH)GetStockObject( BLACK_BRUSH );
  if ( !RegisterClassEx( &wc ) )
  {
    MessageBox( NULL, TEXT( "Program requires Windows NT!" ),
      szAppName, MB_ICONERROR );
    return 0;
  }

  g_hWndMain = CreateWindow( szAppName, TEXT( "RTMPlayer" ),
    WS_OVERLAPPEDWINDOW,
    CW_USEDEFAULT, CW_USEDEFAULT,
    CW_USEDEFAULT, CW_USEDEFAULT,
    NULL, NULL, hInstance, NULL );

  ShowWindow( g_hWndMain, nShowCmd );
  UpdateWindow( g_hWndMain );

  HACCEL hAccelTable = LoadAccelerators( hInstance, MAKEINTRESOURCE( IDR_ACCELERATOR ) );

  while ( GetMessage( &msg, NULL, 0, 0 ) )
  {
    if ( !TranslateAccelerator( msg.hwnd, hAccelTable, &msg ) )
    {
      TranslateMessage( &msg );
      DispatchMessage( &msg );
    }
  }
  return (int)msg.wParam;
}


//std::mutex mx;
VOID
CALLBACK
MyWorkCallback(
  PTP_CALLBACK_INSTANCE Instance,
  PVOID                 params,
  PTP_WORK              Work
)
{
  PARAMS *ptrParams = (PARAMS *)params;
  HWND hWndMain = g_hWndMain;
  struct bufferevent *bev = g_bev;
  Packet *ptrPacket = ptrParams->ptrPacket;
  int flags = ptrParams->flags;
  size_t hashVal = hash_value( ptrPacket->app() );
  TCHAR szBuffer[MAX_PACKET_SIZE];
  if ( flags & EV_READ )
  {
#if 1
    uint64_t recvTimestamp = get_timestamp_ms();
    RTMP_Log( RTMP_LOGDEBUG, "recv packet(%d) from %s, %dB:[%u,%u-%u], packet timestamp=%llu, recv timestamp=%llu, R-P=%llu",
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
      assert( sUnorderedMapHashParams.count( hashVal ) );
      RtmpWindow *ptrRtmpWindow = sUnorderedMapHashParams[hashVal];
      ptrRtmpWindow->pri_queue().push( PacketUtils::new_packet( *ptrPacket ) );
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
      RTMP_Log( RTMP_LOGDEBUG, "OnlineSessions from server: %s", szBuffer );
      std::vector<std::string> appArry = SplitString( ptrPacket->body(), "\n" );
      for ( int i = 0; i < appArry.size(); ++i )
      {
        RtmpWindow *ptrRtmpWindow = new RtmpWindow( NULL, appArry[i], 0, uuid32() );
        size_t hashVal = hash_value( appArry[i] );
        sUnorderedMapHashParams.insert( std::make_pair( hashVal, ptrRtmpWindow ) );
        PostMessage( hWndMain, WM_NEW_ONLINEUSER, hashVal, 0 );
      }
      break;
    }
    case NewSession:
    {
      RTMP_Log( RTMP_LOGDEBUG, "NewSession from %s", ptrPacket->app().c_str() );
      assert( sUnorderedMapHashParams.count( hashVal ) == 0 );
      RtmpWindow *ptrRtmpWindow = new RtmpWindow( NULL, ptrPacket->app(), 0, uuid32() );
      sUnorderedMapHashParams.insert( std::make_pair( hashVal, ptrRtmpWindow ) );
      PostMessage( hWndMain, WM_NEW_ONLINEUSER, hashVal, 0 );
      break;
    }
    case LostSession:
    {
//       RTMP_Log( RTMP_LOGDEBUG, "LostSession from %s", ptrPacket->app().c_str() );
//       assert( sUnorderedMapHashParams.count( hashVal ));
// 			RtmpWindow *ptrRtmpWindow = sUnorderedMapHashParams[ hashVal ];
// 			sUnorderedMapHashParams.erase( hashVal );
// 			PostMessage( hWndMain, WM_DEL_ONLINEUSER, ( WPARAM ) ptrRtmpWindow, 0 );
      break;
    }
    case BuildConnect:
    {
      RTMP_Log( RTMP_LOGDEBUG, "BuildConnect from %s", ptrPacket->app().c_str() );
      assert( sUnorderedMapHashParams.count( hashVal ) );
      RtmpWindow *ptrRtmpWindow = sUnorderedMapHashParams[hashVal];

      PostMessage( hWndMain, WM_NEW_CONNECTION, (WPARAM)ptrRtmpWindow, 0 );

      break;
    }
    case Accept:
    {
      RTMP_Log( RTMP_LOGDEBUG, "Accept from %s", ptrPacket->app().c_str() );
      assert( sUnorderedMapHashParams.count( hashVal ) );
      RtmpWindow *ptrRtmpWindow = sUnorderedMapHashParams[hashVal];
      ptrRtmpWindow->set_timebase( ptrPacket->timebase() );

      PostMessage( hWndMain, WM_NEW_CONNECTION, (WPARAM)ptrRtmpWindow, 1 );

      //ptrRtmpWindow->app = ptrPacket->app();
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

inline void SubmitWork( int flags, const Packet& packet )
{
  if ( flags & EV_READ )
  {
    PARAMS *ptrParams = new PARAMS;
    ptrParams->flags = flags;
    ptrParams->ptrPacket = PacketUtils::new_packet( packet );
    PTP_WORK work = CreateThreadpoolWork( MyWorkCallback, ptrParams, NULL );
    SubmitThreadpoolWork( work );
  }
  else if ( flags & EV_WRITE )
  {
    g_outputQue.push( PacketUtils::new_packet( packet ) );
    bufferevent_enable( g_bev, EV_WRITE );
  }
}

void ReadCallback( struct bufferevent *bev, void *arg )
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

void WriteCallback( struct bufferevent *bev, void *arg )
{
  int ret = 0;
  while ( !g_outputQue.empty() )
  {
    Packet* ptrPacket = g_outputQue.top();
    g_outputQue.pop();
    PACKET rawNetPacket = ptrPacket->raw_net_packet();
    ret = bufferevent_write( bev, (void *)&rawNetPacket, MAX_PACKET_SIZE );
    if ( ret < 0 )
    {
      g_outputQue.push( ptrPacket );
      break;
    }
    delete ptrPacket;
  }
}

void EventCallback( struct bufferevent *bev, short error, void *arg )
{
  if ( error & BEV_EVENT_EOF )
  {
    MessageBox( NULL, TEXT( "connection has closed!" ),
      szAppName, MB_ICONINFORMATION );
  }
  else if ( error & BEV_EVENT_ERROR )
  {
    MessageBox( NULL, TEXT( "error has ocurred!" ),
      szAppName, MB_ICONERROR );
  }
  else if ( error & BEV_EVENT_TIMEOUT )
  {
    MessageBox( NULL, TEXT( "timeout!" ),
      szAppName, MB_ICONINFORMATION );
  }
}

unsigned CALLBACK thread_func_for_dispatcher( void* arg )
{
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
    MessageBox( NULL, TEXT( "Cannot connect to server!" ), szAppName, MB_ICONERROR );
    return 0;
  }
  // use libevent in multi-thread environment
  evthread_use_windows_threads();

  evutil_make_socket_nonblocking( fdClient );
  int32_t slen = sizeof( sin );
  getsockname( fdClient, (struct sockaddr *)&sin, &slen );
  g_app = ::inet_ntoa( sin.sin_addr ) +
    std::string( ":" ) +
    std::to_string( ntohs( sin.sin_port ) );
  TCHAR szBuffer[128];
  wsprintf( szBuffer, "%s(%s)", szAppName, g_app.c_str() );
  SetWindowText( g_hWndMain, szBuffer );

  g_base = event_base_new();
  g_bev = bufferevent_socket_new( g_base,
    fdClient,
    BEV_OPT_CLOSE_ON_FREE );
  bufferevent_setcb( g_bev, ReadCallback, WriteCallback, EventCallback, NULL );
  bufferevent_enable( g_bev, EV_READ | EV_WRITE | EV_PERSIST );

  // C->S: NewSession
  SubmitWork( EV_WRITE, PacketUtils::new_session_packet( get_timestamp_ms(), g_app ) );

  event_base_dispatch( g_base );

  /*
   * cleanup libevent objects
   */
  if ( g_bev )
    bufferevent_free( g_bev );
  if ( g_base )
    event_base_free( g_base );
  return 0;
}

unsigned CALLBACK thread_func_for_encoder( void *arg )
{
  /*
   * begin encoding
   */
  RTMP_Log( RTMP_LOGDEBUG, "encoder thread is start..." );
#if DEBUG_NO_ENCODING
#if DEBUG_DOUBLE_CHECK
  HANDLE hFile = CreateFile( "enc_str.bin", GENERIC_READ, FILE_SHARE_READ, NULL,
    OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL );
#else
  HANDLE hFile = CreateFile( "../../../../0110random/enc_str.bin", GENERIC_READ, FILE_SHARE_READ, NULL,
    OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL );
#endif
  DWORD dwFileSize, dwFileSizeHigh, dwNumberOfBytesRead;
  dwFileSize = GetFileSize( hFile, &dwFileSizeHigh );
  CHAR *szBuffer = (CHAR *)malloc( dwFileSize );
  BOOL bSuccess = ReadFile( hFile, szBuffer, dwFileSize, &dwNumberOfBytesRead, 0 );
  CloseHandle( hFile );


  //   size_t numPack = NUM_PACK( dwFileSize );
  //   for ( size_t i = 0; i < numPack; ++i )
  //   {
  //     SubmitWork( EV_WRITE, PacketUtils::push_packet(
  //       ( uint32_t ) dwFileSize, i != numPack - 1,
  //       ( uint32_t ) i * MAX_BODY_SIZE,
  //       get_timestamp_ms(),
  //       g_app,
  //       ( char * ) szBuffer + i * MAX_BODY_SIZE ) );
  //   }


  CHAR *p = szBuffer, *pL = NULL;
  int numBytes = 0;
  RtmpWindow *ptrRtmpWindow = (RtmpWindow *)arg;
  //ptrRtmpWindow->pri_queue().push( PacketUtils::new_packet( *ptrPacket ) );
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
          ptrRtmpWindow->pri_queue().push( PacketUtils::new_push_packet(
            (uint32_t)frameSize, i != numPack - 1,
            (uint32_t)i * MAX_BODY_SIZE,
            timestamp,
            g_app,
            (char *)pL + i * MAX_BODY_SIZE ) );
#else
          SubmitWork( EV_WRITE, PacketUtils::push_packet(
            (uint32_t)frameSize, i != numPack - 1,
            (uint32_t)i * MAX_BODY_SIZE,
            timestamp,
            g_app,
            (char *)pL + i * MAX_BODY_SIZE ) );
#endif
        }
        msleep( 1000 / 25 );
      }
      pL = p + 1;
    }
    p++;
  }
#if DEBUG_SEND_TO_MYSELF
  ptrRtmpWindow->pri_queue().push( PacketUtils::new_fin_packet( get_timestamp_ms(), g_app ) );
#else
  SubmitWork( EV_WRITE, PacketUtils::fin_packet( get_timestamp_ms(), g_app ) );
#endif
  free( szBuffer );
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

unsigned CALLBACK thread_func_for_decoder( void *arg )
{
  RTMP_Log( RTMP_LOGDEBUG, "decoder thread is start...\n" );
  RtmpWindow* ptrRtmpWindow = (RtmpWindow*)arg;

  int argc = 5;
#if DEBUG_DOUBLE_CHECK
  const char *argv[5] = {
    "",
  "-b",
  "dec_str.bin",
  "-o",
  "dec_rec.yuv" };
#else
  const char *argv[5] = {
  "",
  "-b",
  "../../../../0110random/dec_str.bin",
  "-o",
  "../../../../0110random/dec_rec.yuv" };
#endif

  int returnCode = EXIT_SUCCESS;

  // print information
  RTMP_Log( RTMP_LOGDEBUG, "\n" );
  RTMP_Log( RTMP_LOGDEBUG, "VVCSoftware: VTM Decoder Version %s ", VTM_VERSION );
  RTMP_Log( RTMP_LOGDEBUG, NVM_ONOS );
  RTMP_Log( RTMP_LOGDEBUG, NVM_COMPILEDBY );
  RTMP_Log( RTMP_LOGDEBUG, NVM_BITS );
#if ENABLE_SIMD_OPT
  std::string SIMD;
  df::program_options_lite::Options optsSimd;
  optsSimd.addOptions()("SIMD", SIMD, string( "" ), "");
  df::program_options_lite::SilentReporter err;
  df::program_options_lite::scanArgv( optsSimd, argc, (const char**)argv, err );
  RTMP_Log( RTMP_LOGDEBUG, "[SIMD=%s] ", read_x86_extension( SIMD ) );
#endif
#if ENABLE_TRACING
  RTMP_Log( RTMP_LOGDEBUG, "[ENABLE_TRACING] " );
#endif
  RTMP_Log( RTMP_LOGDEBUG, "\n" );

  DecApp *pcDecApp = new DecApp;
  // parse configuration
  if ( !pcDecApp->parseCfg( argc, (char **)argv ) )
  {
    returnCode = EXIT_FAILURE;
    return 0;
  }

  // starting time
  double dResult;
  clock_t lBefore = clock();

  // call decoding function
#ifndef _DEBUG
  try
  {
#endif // !_DEBUG
    if ( 0 != pcDecApp->decode( ptrRtmpWindow ) )
    {
      RTMP_Log( RTMP_LOGDEBUG, "\n\n***ERROR*** A decoding mismatch occured: signalled md5sum does not match\n" );
      returnCode = EXIT_FAILURE;
    }
#ifndef _DEBUG
  }
  catch ( Exception &e )
  {
    std::cerr << e.what() << std::endl;
    returnCode = EXIT_FAILURE;
  }
  catch ( const std::bad_alloc &e )
  {
    std::cout << "Memory allocation failed: " << e.what() << std::endl;
    returnCode = EXIT_FAILURE;
  }
#endif

  // ending time
  dResult = (double)(clock() - lBefore) / CLOCKS_PER_SEC;
  RTMP_Log( RTMP_LOGDEBUG, "\n Total Time: %12.3f sec.\n", dResult );

  delete pcDecApp;

  RTMP_Log( RTMP_LOGDEBUG, "decoder thread is quit.\n" );
  return 0;
}

INT_PTR CALLBACK RequestDlgProc( HWND hDlg, UINT message,
  WPARAM wParam, LPARAM lParam )
{
  TCHAR szBuffer[128];
  HWND hWndParent = GetParent( hDlg );
  switch ( message )
  {
  case WM_INITDIALOG:
  {
    HMENU hMenu = GetMenu( hWndParent );
    TCHAR szMenuStr[128];
    GetMenuString( hMenu, LOWORD( lParam ), szMenuStr, sizeof( szMenuStr ), MF_BYCOMMAND );
    wsprintf( szBuffer, "%s want to video with you?", szMenuStr );
    HWND hStaticConfirm = GetDlgItem( hDlg, IDC_STATIC_REQUEST );
    SetWindowText( hStaticConfirm, szBuffer );
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

INT_PTR CALLBACK ConfirmDlgProc( HWND hDlg, UINT message,
  WPARAM wParam, LPARAM lParam )
{
  TCHAR szBuffer[128];
  HWND hWndParent = GetParent( hDlg );
  switch ( message )
  {
  case WM_INITDIALOG:
  {
    HMENU hMenu = GetMenu( hWndParent );
    TCHAR szMenuStr[128];
    GetMenuString( hMenu, LOWORD( lParam ), szMenuStr, sizeof( szMenuStr ), MF_BYCOMMAND );
    wsprintf( szBuffer, "Are you sure video with %s?", szMenuStr );
    HWND hStaticConfirm = GetDlgItem( hDlg, IDC_STATIC_CONFIRM );
    SetWindowText( hStaticConfirm, szBuffer );
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

LRESULT CALLBACK WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
  HDC hdc;
  PAINTSTRUCT ps;
  static PTP_POOL ptpp;
  static PTP_CALLBACK_ENVIRON pcbe;
  static HANDLE hDispatcherThread;
  TCHAR szBuffer[256];
  HMENU hMenu = GetMenu( hWnd );
  static int cxClient, cyClient;
  switch ( message )
  {
  case WM_CREATE:
  {
#if _WIN32
    WSADATA wsaData;
    WSAStartup( MAKEWORD( 2, 2 ), &wsaData );
#endif
    /*
     * window initialization
     */
    HMENU hMenuView = GetSubMenu( hMenu, 0 );
    HMENU hMenuOnlineUser = CreatePopupMenu();
    InsertMenu( hMenuView, 0, MF_POPUP, (UINT_PTR)hMenuOnlineUser, "&Online Users\tCtrl+O" );
    DWORD dwCxScreen = GetSystemMetrics( SM_CXSCREEN );
    DWORD dwCyScreen = GetSystemMetrics( SM_CYSCREEN );
#define CXPLAYER 1000
#define CYPLAYER 700
    MoveWindow( hWnd, (dwCxScreen - CXPLAYER) / 2, (dwCyScreen - CYPLAYER) / 2,
      CXPLAYER, CYPLAYER, TRUE );

    /*
     * Create Log Thread
     */
    FILE* dumpfile = fopen( "RTMPlayer.dump", "a+" );
    RTMP_LogSetOutput( dumpfile );
    RTMP_LogSetLevel( RTMP_LOGALL );
    RTMP_LogThreadStart();
    SYSTEMTIME tm;
    GetSystemTime( &tm );
    RTMP_Log( RTMP_LOGDEBUG, "==============================" );
    RTMP_Log( RTMP_LOGDEBUG, "log file:\tRTMPlayer.dump" );
    RTMP_Log( RTMP_LOGDEBUG, "log timestamp:\t%lld", get_timestamp_ms() );
    RTMP_Log( RTMP_LOGDEBUG, "log date:\t%d-%d-%d %d:%d:%d",
      tm.wYear,
      tm.wMonth,
      tm.wDay,
      tm.wHour, tm.wMinute, tm.wSecond );
    RTMP_Log( RTMP_LOGDEBUG, "==============================" );

    /*
     * Create Thread Pool
     */
    SYSTEM_INFO sysinfo;
    GetSystemInfo( &sysinfo );
    int numOfProcessors = sysinfo.dwNumberOfProcessors;
    ptpp = CreateThreadpool( NULL );
    SetThreadpoolThreadMaximum( ptpp, 1/*numOfProcessors*/ );
    SetThreadpoolThreadMinimum( ptpp, 1 );

    /*
     * start event thread
     * use _beginthread, it will automatically stopping when exit
     */
    hDispatcherThread = (HANDLE)_beginthreadex( NULL, 0, thread_func_for_dispatcher, NULL, 0, NULL );



    /*
     * Create Child Windows
     */

    break;
  }
  case WM_SIZE:
  {
    cxClient = LOWORD( lParam );
    cyClient = HIWORD( lParam );

    /* 
     * child window size setting
     * TODO: adjust for multiple child window
     */
    int childWinNum = sUnorderedMapHashParams.size();
    // caculating how much size for each child window
    for (auto it = sUnorderedMapHashParams.begin(); it != sUnorderedMapHashParams.end(); ++it)
    {
      RtmpWindow *pRtmpWindow = it->second;
      MoveWindow( pRtmpWindow->win(), 0, 0, cxClient, cyClient, TRUE );
    }
    break;
  }
  case WM_NEW_ONLINEUSER:
  {
    size_t hashVal = wParam;
    assert( sUnorderedMapHashParams.count( hashVal ) );
    RtmpWindow *ptrRtmpWindow = sUnorderedMapHashParams[hashVal];

    HMENU hMenuView = GetSubMenu( hMenu, 0 ); 
    HMENU hMenuOnlineUsers = GetSubMenu( hMenuView, 0 );
    AppendMenu( hMenuOnlineUsers, MF_STRING, ptrRtmpWindow->menuid(), ptrRtmpWindow->app().c_str() );
    break;
  }
  case WM_DEL_ONLINEUSER:
  {
    RtmpWindow *ptrRtmpWindow = (RtmpWindow *)wParam;
    DeleteMenu( hMenu, ptrRtmpWindow->menuid(), MF_BYCOMMAND );
    if (ptrRtmpWindow )
      delete ptrRtmpWindow;

    // no user, stop encoding
    if ( sUnorderedMapHashParams.empty() )
    {
      //TerminateThread( hDispatcherThread, 0 );
      //g_bIsEncoding = FALSE;
    }
    break;
  }
  case WM_NEW_CONNECTION:
  {
    /*
     * connection has build, you need prepare to receive and push data
     * wParam: RtmpWindow
     * lParam: 0: receive BuildConnect, 1: receive Accept
     */
    RtmpWindow *ptrRtmpWindow = (RtmpWindow *)wParam;
    if ( lParam == 0 ) // receive BuildConnect
    {
      if ( DialogBoxParam( g_hIns, MAKEINTRESOURCE( IDD_DIALOG_REQUEST ), hWnd,
        RequestDlgProc, ptrRtmpWindow->menuid() ) == IDOK )
      {
        SubmitWork( EV_WRITE, PacketUtils::accept_packet( (uint32_t)ptrRtmpWindow->app().size(), g_app, g_timebase,
          ptrRtmpWindow->app().c_str() ) );
      }
      else break;
    }
    // receive Accept
    ptrRtmpWindow->set_win( CreateWindow( szChildClassName, ptrRtmpWindow->app().c_str(), WS_CHILDWINDOW | WS_VISIBLE/* | WS_BORDER*/,
      0, 0, 0, 0,
      hWnd, (HMENU)uuid32(),
      (HINSTANCE)GetWindowLongPtr( hWnd, GWLP_HINSTANCE ), NULL ) );
    SendMessage( hWnd, WM_SIZE, 0, MAKELPARAM( cxClient, cyClient ) );

    // begin encoding
    if ( !g_bIsEncoding )
    {
      g_bIsEncoding = TRUE;
      g_hEncoderThread = (HANDLE)_beginthreadex( NULL, 0, thread_func_for_encoder, ptrRtmpWindow, 0, NULL );
    }
    break;
  }
  case WM_CONTEXTMENU:
  {
    POINT pt;
    HMENU hSubMenu = GetSubMenu( hMenu, 0 );
    pt.x = LOWORD( lParam );
    pt.y = HIWORD( lParam );
    TrackPopupMenu( hSubMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hWnd, NULL );
    break;
  }
  case WM_COMMAND:
  {
    switch ( LOWORD( wParam ) )
    {
    case ID_VIEW_EXIT:
      DestroyWindow( hWnd );
      break;
    case ID_HELP_ABOUT:
      DialogBox( g_hIns, MAKEINTRESOURCE( IDD_DIALOG_ABOUT ), hWnd, About );
      break;
    default:
    {
      TCHAR szMenuStr[128];
      GetMenuString( hMenu, LOWORD( wParam ), szMenuStr, sizeof( szMenuStr ), MF_BYCOMMAND );
      if ( DialogBoxParam( g_hIns, MAKEINTRESOURCE( IDD_DIALOG_CONFIRM ), hWnd, ConfirmDlgProc, LOWORD( wParam ) ) == IDOK )
      {
        SubmitWork( EV_WRITE, PacketUtils::build_connect_packet( lstrlen( szMenuStr ), g_app, szMenuStr ) );
      }
      break;
    }
    }
    break;
  }
  case WM_PAINT:
  {
    hdc = BeginPaint( hWnd, &ps );
    EndPaint( hWnd, &ps );
    break;
  }
  case WM_DESTROY:
  {
    /*
     * stop dispatcher thread
     */
    event_base_loopbreak( g_base );

    /*
     * stop thread pool
     */
    if ( ptpp )
      CloseThreadpool( ptpp );

    /*
     * stop encoder thread
     */
    if ( g_hEncoderThread )
      CloseHandle( g_hEncoderThread );

    /*
     * destroy child window(include threads in child window )
     */
    for ( auto it = sUnorderedMapHashParams.begin();
      it != sUnorderedMapHashParams.end();
      ++it )
    {
      RtmpWindow *ptrRtmpWindow = it->second;
      delete ptrRtmpWindow;
    }

    /*
     * stop log thread
     */
    RTMP_LogThreadStop();

    PostQuitMessage( 0 );
#if _WIN32
    WSACleanup();
#endif
    break;
  }
  default:
    break;
  }
  return DefWindowProc( hWnd, message, wParam, lParam );
}

HBITMAP YUVtoBitmap( HDC hdc, BYTE *pYUVData, DWORD cxOrigPic, DWORD cyOrigPic )
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

HBITMAP NextFrame( HDC hdc, HANDLE hFile, DWORD cxOrigPic, DWORD cyOrigPic )
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

  return YUVtoBitmap( hdc, pBuffer, cxOrigPic, cyOrigPic );
}

LRESULT CALLBACK ChildWndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
  static int cxClient, cyClient;
  static HANDLE hDecoderThread;
  PAINTSTRUCT ps;
  static DWORD cxOrigPic = 832, cyOrigPic = 480;
  static DWORD cxPic, cyPic;
  static DWORD dwFrameCount;
  static HBITMAP hBitmap;
  static size_t hashVal;
  static RtmpWindow *ptrRtmpWindow;
  TCHAR szBuffer[128];
  HDC hdc;
#if DEBUG_LOCAL_VIDEO_TEST
  static HANDLE hFile;
  TCHAR szFilePath[128] = TEXT( "../../../../0110random/BasketballDrill_832x480_50.yuv" );
#endif
  switch ( message )
  {
  case WM_CREATE:
  {
#if DEBUG_LOCAL_VIDEO_TEST
    hFile = CreateFile( szFilePath, GENERIC_READ, FILE_SHARE_READ, NULL,
      OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL );
    SetTimer( hWnd, 1, 1000 / 25, NULL );
#endif
    /*
     * start decode thread
     * use _beginthread, it will automatically stopping when exit
     */
    GetWindowText( hWnd, szBuffer, sizeof( szBuffer ) );
    hashVal = hash_value( szBuffer );
    assert( sUnorderedMapHashParams.count( hashVal ) );
    ptrRtmpWindow = sUnorderedMapHashParams[hashVal];
    hDecoderThread = (HANDLE)_beginthreadex( NULL, 0, thread_func_for_decoder, ptrRtmpWindow, 0, NULL );
    break;
  }
  case WM_NEW_YUV_FRAME:
  {
    // count frame
    dwFrameCount++;

    Frame *ptrYUVFrame = (Frame *)wParam;
    HDC hdc = GetDC( hWnd );
    hBitmap = YUVtoBitmap( hdc, (BYTE*)ptrYUVFrame->data, cxOrigPic, cyOrigPic );
    ReleaseDC( hWnd, hdc );
    InvalidateRect( hWnd, NULL, FALSE );
    free( ptrYUVFrame->data );
    delete ptrYUVFrame;
    break;
  }
  case WM_TIMER:
  {
#if DEBUG_LOCAL_VIDEO_TEST
    HDC hdc = GetDC( hWnd );
    hBitmap = NextFrame( hdc, hFile, cxOrigPic, cyOrigPic );
    ReleaseDC( hWnd, hdc );
    InvalidateRect( hWnd, NULL, FALSE );
#endif
    break;
  }
  case WM_SIZE:
  {
    cxClient = LOWORD( lParam );
    cyClient = HIWORD( lParam );
    
    // image size adaptive
    float ratio = (float)cxClient / cyClient;
    if ( ratio < (float)cxOrigPic / cyOrigPic )
    {
      if ( cxOrigPic > cxClient )
        cxPic = cxClient;
      else
        cxPic = cxOrigPic;
      cyPic = cxPic * (float)cyOrigPic / cxOrigPic;
    }
    else
    {
      if ( cyOrigPic > cyClient )
        cyPic = cyClient;
      else
        cyPic = cyOrigPic;
      cxPic = cyPic * (float)cxOrigPic / cyOrigPic;
    }

    break;
  }
  case WM_PAINT:
  {
    wsprintf( szBuffer, "Frame Number: %u", dwFrameCount );
    hdc = BeginPaint( hWnd, &ps );
    HDC hdcMem = CreateCompatibleDC( hdc );
    SelectObject( hdcMem, hBitmap );
    SetTextColor( hdcMem, RGB( 255, 255, 255 ) );
    SetBkMode( hdcMem, TRANSPARENT );
    TextOut( hdcMem, 0, 0, szBuffer, lstrlen( szBuffer ) );
    StretchBlt( hdc,
      (cxClient - cxPic) / 2, (cyClient - cyPic) / 2,
      cxPic, cyPic,
      hdcMem,
      0, 0,
      cxOrigPic, cyOrigPic,
      SRCCOPY );
    DeleteDC( hdcMem );
    EndPaint( hWnd, &ps );
    break;
  }
  case WM_DESTROY:
  {
    /*
     * stop decoder thread
     */
    ptrRtmpWindow->set_lost();
    WaitForSingleObject( hDecoderThread, INFINITE );

#if DEBUG_LOCAL_VIDEO_TEST
    CloseHandle( hFile );
#endif
    break;
  }
  default:
    break;
  }
  return DefWindowProc( hWnd, message, wParam, lParam );
}

// Message handler for about box.
INT_PTR CALLBACK About( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam )
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