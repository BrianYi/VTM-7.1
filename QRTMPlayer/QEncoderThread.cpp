#include "EncoderLib/EncLibCommon.h"
#include "EncApp.h"
#include "Utilities/program_options_lite.h"

#include "QEncoderThread.h"
#include "QRTMPlayer.h"


QEncoderThread::QEncoderThread(QObject *parent)
	: QThread(parent)
{
  fRTMPlayer = (QRTMPlayer *)parent;
}

QEncoderThread::~QEncoderThread()
{
}

void QEncoderThread::run()
{
  /*
  * begin encoding
  */
  qDebug() << "encoder thread is start...";
#if DEBUG_NO_ENCODING
#if DEBUG_DOUBLE_CHECK
  HANDLE hFile = CreateFile( "enc_str.bin", GENERIC_READ, FILE_SHARE_READ, NULL,
    OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL );
#else
  HANDLE hFile = CreateFile( TEXT( "../0110random/enc_str.bin" ), GENERIC_READ, FILE_SHARE_READ, NULL,
    OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL );
#endif
  DWORD dwFileSize, dwFileSizeHigh, dwNumberOfBytesRead;
  dwFileSize = GetFileSize( hFile, &dwFileSizeHigh );
  CHAR *szFileData = (CHAR *)malloc( dwFileSize );
  BOOL bSuccess = ReadFile( hFile, szFileData, dwFileSize, &dwNumberOfBytesRead, 0 );
  CloseHandle( hFile );
  CHAR *p = szFileData, *pL = NULL;
  size_t numBytes = 0;
  std::string uuid = fRTMPlayer->uuidStr().toStdString();
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
          emit sigSendPacket( PacketUtils::push_packet(
            (uint32_t)frameSize, i != numPack - 1,
            (uint32_t)i * MAX_BODY_SIZE,
            timestamp,
            uuid,
            (char *)pL + i * MAX_BODY_SIZE ) );
#endif
        }
        this->msleep( fRTMPlayer->timebase() );
      }
      pL = p + 1;
    }
    p++;
  }
#if DEBUG_SEND_TO_MYSELF
  win->PriQueue().push( PacketUtils::new_fin_packet( get_timestamp_ms(), g_app ) );
#else
  emit sigSendPacket( PacketUtils::fin_packet( get_timestamp_ms(), uuid ) );
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
  qDebug() << "[SIMD=%s] " << read_x86_extension( SIMD ) ;
#endif
#if ENABLE_TRACING
  qDebug() << "[ENABLE_TRACING] " );
#endif
#if ENABLE_SPLIT_PARALLELISM
  qDebug() << "[SPLIT_PARALLEL (%d jobs)]", PARL_SPLIT_MAX_NUM_JOBS );
#endif
#if ENABLE_SPLIT_PARALLELISM
  const char* waitPolicy = getenv( "OMP_WAIT_POLICY" );
  const char* maxThLim = getenv( "OMP_THREAD_LIMIT" );
  qDebug() << waitPolicy ? "[OMP: WAIT_POLICY=%s," : "[OMP: WAIT_POLICY=,", waitPolicy );
  qDebug() << maxThLim ? "THREAD_LIMIT=%s" : "THREAD_LIMIT=", maxThLim );
  qDebug() << "]" );
#endif
  qDebug() << "\n" ;

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
        return ;
      }
    }
    catch ( df::program_options_lite::ParseFailure &e )
    {
      std::cerr << "Error parsing option \"" << e.arg << "\" with argument \"" << e.val << "\"." << std::endl;
      return ;
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
  qDebug() << " started @ %s", std::ctime( &startTime2 );
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

  qDebug() << "\n finished @ %s" << std::ctime( &endTime2 ) ;

#if JVET_O0756_CALCULATE_HDRMETRICS
  qDebug() << " Encoding Time (Total Time): %12.3f ( %12.3f ) sec. [user] %12.3f ( %12.3f ) sec. [elapsed]\n",
    ((endClock - startClock) * 1.0 / CLOCKS_PER_SEC) - (metricTimeuser / 1000.0),
    (endClock - startClock) * 1.0 / CLOCKS_PER_SEC,
    encTime / 1000.0,
    totalTime / 1000.0 );
#else
  qDebug() << " Total Time: %12.3f sec. [user] %12.3f sec. [elapsed]\n" <<
    (endClock - startClock) * 1.0 / CLOCKS_PER_SEC <<
    encTime / 1000.0 ;
#endif
#endif
 // return 0;
}
