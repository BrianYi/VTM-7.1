#include "KHeader.h"

namespace K
{
  KMutex::KMutex()
  {
    ::InitializeCriticalSection( &fMutex );
    fHolder = 0;
    fHolderCount = 0;
  }

  KMutex::~KMutex()
  {
    ::DeleteCriticalSection( &fMutex );
  }

  void KMutex::Lock()
  {
    this->RecursiveLock();
  }

  void KMutex::Unlock()
  {
    this->RecursiveUnlock();
  }

  BOOL KMutex::TryLock()
  {
    return this->RecursiveTryLock();
  }

  void KMutex::RecursiveLock()
  {
    if ( ::GetCurrentThreadId() == fHolder )
    {
      fHolderCount++;
      return;
    }

    ::EnterCriticalSection( &fMutex );
    assert( fHolder == 0 );
    fHolder = ::GetCurrentThreadId();
    fHolderCount++;
    assert( fHolderCount == 1 );

  }

  void KMutex::RecursiveUnlock()
  {
    if ( ::GetCurrentThreadId() != fHolder )
      return;

    assert( fHolderCount > 0 );

    fHolderCount--;
    if ( fHolderCount == 0 )
    {
      fHolder = 0;
      ::LeaveCriticalSection( &fMutex );
    }
  }

  BOOL KMutex::RecursiveTryLock()
  {
    if ( ::GetCurrentThreadId() == fHolder )
    {
      fHolderCount++;
      return TRUE;
    }

    BOOL theErr = ::TryEnterCriticalSection( &fMutex );
    if ( !theErr ) return theErr;

    assert( fHolder == 0 );
    fHolder = ::GetCurrentThreadId();
    fHolderCount++;
    assert( fHolderCount == 1 );
    return TRUE;
  }


};