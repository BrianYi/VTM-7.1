#pragma once
#include "KObject.h"

namespace K
{
  class KLabel :
    public KObject
  {
  public:
    KLabel():
      KObject()
    {}
    virtual ~KLabel()
    {}
    virtual const TCHAR* GetClassName() { return _T( "S_STATIC" ); };
  };

};