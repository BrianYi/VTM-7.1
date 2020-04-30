#include "DecoderApp/DecApp.h"
#include "program_options_lite.h"
#include "RtmpWindow.h"

unsigned CALLBACK thread_func_for_decoder( void *arg )
{
  RTMP_Log( RTMP_LOGDEBUG, "decoder thread is start...\n" );
  RtmpWindow* win = (RtmpWindow*)arg;

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
    if ( 0 != pcDecApp->decode( win ) )
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

RtmpWindow::RtmpWindow( HWND win, std::string app, int32_t timebase, uint32_t menuid ) :
  fHwnd( win ),
  fApp( app ),
  fTimebase( timebase ),
  fMenuId( menuid ),
  fLostConnection( false )
{
  /*
  * start decode thread
  * use _beginthread, it will automatically stopping when exit
  */
  fHDecoderThread = (HANDLE)_beginthreadex( NULL, 0, thread_func_for_decoder, this, 0, NULL );
}

RtmpWindow::~RtmpWindow()
{
  /*
     * stop decoder thread
     */
  this->set_lost();
  WaitForSingleObject( fHDecoderThread, INFINITE );

  /*
   * destroy window
   */
  DestroyWindow( fHwnd );
  while ( !fPriQue.empty() )
  {
    Packet *packet = fPriQue.top();
    fPriQue.pop();
    delete packet;
  }
}