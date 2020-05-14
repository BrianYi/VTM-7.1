#pragma once

namespace K
{
  class KMutex
  {
  public:
    KMutex();
    ~KMutex();
    void Lock();
    void Unlock();
    BOOL TryLock();
    void RecursiveLock();
    void RecursiveUnlock();
    BOOL RecursiveTryLock();
  private:
    DWORD fHolder;
    DWORD fHolderCount;
    CRITICAL_SECTION fMutex;
  };

  class KMutexLocker
  {
  public:
    KMutexLocker( KMutex *inMutex ) :
      fMutex( inMutex )
    {
      if ( fMutex )
        fMutex->Lock();
    }
    ~KMutexLocker()
    {
      if ( fMutex )
        fMutex->Unlock();
    }
    void Lock()
    {
      if ( fMutex )
        fMutex->Lock();
    }
    void Unlock()
    {
      if ( fMutex )
        fMutex->Unlock();
    }
  private:
    KMutex* fMutex;
  };
};