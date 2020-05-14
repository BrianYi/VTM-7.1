#pragma once
#include "KHeader.h"

namespace K
{
  class KCond
  {
  public:
    KCond()
    {
      fCondition = ::CreateEvent( NULL, FALSE, FALSE, NULL );
      assert( fCondition );
    }
    ~KCond()
    {
      BOOL theErr = ::CloseHandle( fCondition );
      assert( theErr == TRUE );
    }
    void Signal()
    {
      BOOL theErr = ::SetEvent( fCondition );
      assert( theErr == TRUE );
    }
    void Wait( KMutex* inMutex, INT32 inTimeoutInMilSecos = 0 )
    {
      DWORD theTimeout = INFINITE;
      if ( inTimeoutInMilSecos > 0 )
        theTimeout = inTimeoutInMilSecos;
      inMutex->Unlock();
      fWaitCount++;
      DWORD theErr = ::WaitForSingleObject( fCondition, theTimeout );
      fWaitCount--;
      assert( (theErr == WAIT_OBJECT_0) || (theErr == WAIT_TIMEOUT) );
      inMutex->Lock();
    }
    void Broadcase()
    {
      UINT32 waitCount = fWaitCount;
      BOOL theErr = TRUE;
      for ( UINT32 i = 0; i < waitCount; ++i )
      {
        theErr = ::SetEvent( fCondition );
        assert( theErr == TRUE );
      }
    }
  private:
    HANDLE fCondition;
    UINT32 fWaitCount;
  };

};