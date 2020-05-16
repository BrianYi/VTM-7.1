#include "QDecoderThread.h"
#include "EncoderLib/EncLibCommon.h"
#include "EncoderApp/EncApp.h"
#include "DecoderApp/DecApp.h"
#include "Utilities/program_options_lite.h"
#include <QDebug>

QDecoderThread::QDecoderThread( QRtmpWindow *rtmpWindow )
{
  fRtmpWindow = rtmpWindow;
}

QDecoderThread::~QDecoderThread()
{

}

void QDecoderThread::run()
{
  int argc = 5;
#if DEBUG_DOUBLE_CHECK
  const char *argv[5] = {
    "",
  "-b",
  "dec_str.bin",
  "-o",
  "dec_rec.yuv"
  };
#else
  const char *argv[5] = {
  "",
  "-b",
  "../0110random/dec_str.bin",
  "-o",
  "../0110random/dec_rec.yuv" };
#endif

  int returnCode = EXIT_SUCCESS;

  // print information
  qDebug() << "\n" ;
  qDebug() << "VVCSoftware: VTM Decoder Version %s ", VTM_VERSION ;
  qDebug() << NVM_ONOS ;
  qDebug() << NVM_COMPILEDBY ;
  qDebug() << NVM_BITS ;
#if ENABLE_SIMD_OPT
  std::string SIMD;
  df::program_options_lite::Options optsSimd;
  optsSimd.addOptions()("SIMD", SIMD, string( "" ), "");
  df::program_options_lite::SilentReporter err;
  df::program_options_lite::scanArgv( optsSimd, argc, (const char**)argv, err );
  qDebug() << "[SIMD=%s] " << read_x86_extension( SIMD ) ;
#endif
#if ENABLE_TRACING
  qDebug() << "[ENABLE_TRACING] " );
#endif
  qDebug() << "\n" ;

  DecApp *pcDecApp = new DecApp;
  // parse configuration
  if ( !pcDecApp->parseCfg( argc, (char **)argv ) )
  {
    returnCode = EXIT_FAILURE;
    return ;
  }

  // starting time
  double dResult;
  clock_t lBefore = clock();

  // call decoding function
#ifndef _DEBUG
  try
  {
#endif // !_DEBUG
    if ( 0 != pcDecApp->decode( fRtmpWindow ) )
    {
      qDebug() << "\n\n***ERROR*** A decoding mismatch occured: signalled md5sum does not match\n";
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
  qDebug() << "\n Total Time: %12.3f sec.\n" << dResult;

  delete pcDecApp;

  qDebug() << "decoder thread is quit.\n";
  return ;
}
