#pragma once
#include "Packet.h"
#include "PlayerHeader.h"
#include "KHeader.h"
#include <unordered_set>

struct event_base;
struct bufferevent;
namespace K
{
  class KObject;

  /*
     * Log related
     */
  typedef enum
  {
    LOGCRIT = 0, LOGERROR, LOGWARNING, LOGINFO,
    LOGDEBUG, LOGDEBUG2, LOGALL
  } LogLevel;

  class KUtils
  {
    friend KObject;
  public:
    /*
     * Initialize only once
     * UUID is global unique identifier
     */
    static BOOL Initialize(HINSTANCE inst, const TCHAR* szResPath, const TCHAR* szProgName);
    static BOOL CleanUp();
    static void SetMainWindow( KObject *kWindow ) { sMainWindow = kWindow; }
    static KObject *GetMainWindow() { return sMainWindow; }
    //static BOOL SetClass( const TCHAR* lpszClassName );
    //static BOOL CountClass( const TCHAR* lpszClassName );
    //static UINT64 GetUUID() { return sUUID64; }
    //static const TCHAR* GetUUIDStr() { return sUUID64Str; }
    static void SetInstance( HINSTANCE inst ) { sInst = inst; }
    static HINSTANCE GetInstance() { return sInst; }
    static HBITMAP LoadImage( const TCHAR* szFileName );
    static void SetResourcePath( const TCHAR* szResPath ) { ::lstrcpy(sResPath,szResPath); }
    static const TCHAR* GetResourcePath() { return sResPath; }
    static void SetProgramName( const TCHAR* szProgName ) { ::lstrcpy( sProgName, szProgName ); }
    static const TCHAR* GetProgramName() { return sProgName; }
    static KObject *GetWindow( const UINT64& uUUID);
    
    /*
     * Tools
     */
    static UINT64 GetTimestampMS();
    static UINT64 GetTimestampS();
    static UINT64 GenUUID();
    //static UINT32 GenUUID();
    static INT32 GetCommandID( const UINT64& uUUID );
    static INT32 GetCommandIDBase() { return 100000; }
    static BOOL IsCommandID( const INT32& iCommandID ) { return (BOOL)GetUUID( iCommandID ); }
    static UINT64 GetUUID( const INT32& iCommandID );
    static BOOL GetUUIDStr( TCHAR* szBuffer, const INT32& iCommandID );
    static std::string ToString( const UINT64& uINT64 );
    static std::string ToString( const UINT32& uINT32 );
    static INT64 ToInt64( const KString& kStr );
    static INT32 ToInt32( const KString& kStr );
    static HBITMAP YUVtoBitmap( HDC hdc, BYTE *pYUVData, DWORD cxOrigPic, DWORD cyOrigPic );
    static std::vector<std::string> SplitString( std::string sourStr, std::string delimiter );
    static void sleep( const UINT32& miliseconds ) { ::Sleep( miliseconds ); }
    static BOOL LogSetFile( FILE *file );
    static BOOL LogSetLevel( LogLevel logLevel );
    static BOOL LogStart();
    static BOOL LogStop();
    static BOOL Log( LogLevel logLevel, const char *format, ... );
    static BOOL LogAndPrint( LogLevel logLevel, const char *format, ... );
    static BOOL WaitDispatcherStartFinished();
    static void SubmitWork( int flags, const Packet& packet );
  private:
    static BOOL SetWindow( KObject *obj );
    static unsigned CALLBACK DispatcherThread( void* arg );
    static void ReadCallback( struct bufferevent *bev, void *arg );
    static void WriteCallback( struct bufferevent *bev, void *arg );
    static void EventCallback( struct bufferevent *bev, short error, void *arg );
    static void CALLBACK ReceiveCallback( PTP_CALLBACK_INSTANCE Instance, PVOID params, PTP_WORK Work );
  private:
    static HINSTANCE sInst;
    static TCHAR sResPath[MAX_PATH];
    static TCHAR sProgName[MAX_PATH];
    static FILE *sLogFile;
    static LogLevel sLogLevel;
    static UINT64 sUUID64Counter;
//    static UINT32 sUUID32Counter;
    static INT32 sCommandIDCounter;
    static KMutex sUtilsMutex;
    static KCond sDispatcherCond;
    static KMutex sDispatcherMutex;
    //     struct Session
//     {
//       KRtmpWindow *win; // don't delete it! dangerous
//       uint32_t menuid;
//       std::string app;
//       size_t hash;
//     };
    static struct event_base* sEventBase;
    static struct bufferevent* sBufferevent;
    static PTP_POOL sPTPPool;
    static PTP_CALLBACK_ENVIRON sPTPCallEnv;
    static HANDLE sDispatcherThread;
    static PriorityQueue sOutputQueue;
    static KObject *sMainWindow;
    static std::unordered_map<UINT64, KObject*> sHashUUIDtoWin;
    static std::unordered_map<UINT64, INT32> sUUIDtoCommandID;
 //   static std::unordered_set<std::string> sClassSet;

 //   static std::unordered_map<UINT64, Session*> sHashUUIDtoSession;
  };
};