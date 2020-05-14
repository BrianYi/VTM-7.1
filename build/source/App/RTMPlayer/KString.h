#pragma once
#include "KHeader.h"
namespace K
{
  class KString
  {
  public:
    // constructors
    KString()
    {
      fCapacity = 0;
      fData = nullptr;
      fLength = 0;
    }
    KString( const KString& str )
    {
      fCapacity = Caculate( str.Length() + 1 );
      fData = (TCHAR *)malloc( fCapacity * sizeof( TCHAR ) );
      lstrcpy( fData, str.Data() );
      fLength = str.Length();
    }
    KString( TCHAR *szStr )
    {
      UINT32 uLen = lstrlen( szStr );
      fCapacity = Caculate( uLen + 1 );
      fData = (TCHAR *)malloc( fCapacity * sizeof( TCHAR ) );
      lstrcpy( fData, szStr );
      fLength = lstrlen( fData );
    }
    KString( const UINT64& uDigit64 )
    {
      if ( uDigit64 >> 63 )
        fCapacity = 128;
      else if ( uDigit64 >> 31 )
        fCapacity = 64;
      else
        fCapacity = 32;

      fData = (TCHAR*)malloc( fCapacity * sizeof( TCHAR ) );
      wsprintf( fData, _T( "%I64u" ), uDigit64 );
      fLength = lstrlen( fData );
    }
    KString( const UINT32& uDigit32 )
    {
      if ( uDigit32 >> 31 )
        fCapacity = 64;
      else
        fCapacity = 32;

      fData = (TCHAR*)malloc( fCapacity * sizeof( TCHAR ) );
      wsprintf( fData, _T( "%I32u" ), uDigit32 );
      fLength = lstrlen( fData );
    }
    KString( const UINT16& uDigit16 )
    {
      if ( uDigit16 >> 15 )
        fCapacity = 32;
      else
        fCapacity = 16;

      fData = (TCHAR*)malloc( fCapacity * sizeof( TCHAR ) );
      wsprintf( fData, _T( "%I16u" ), uDigit16 );
      fLength = lstrlen( fData );
    }

    // operators
    KString& operator=( TCHAR *szStr )
    {
      UINT32 uLength = lstrlen( szStr );
      UINT32 uNewCapacity = Caculate( uLength + 1 );

      if ( fData )
      {
        if ( fCapacity < uNewCapacity )
        {
          free( fData );
          fCapacity = uNewCapacity;
          fData = (TCHAR *)malloc( fCapacity * sizeof( TCHAR ) );
        }
      }
      else
      {
        fCapacity = uNewCapacity;
        fData = (TCHAR *)malloc( fCapacity * sizeof( TCHAR ) );
      }

      lstrcpy( fData, szStr );
      fLength = lstrlen( fData );
      return *this;
    }
    KString& operator=( const KString& str )
    {
      *this = str.Data();
      return *this;
    }
    KString operator+( TCHAR *szStr )
    {
      UINT32 uLength = lstrlen( szStr );
      UINT32 uNewCapacity = Caculate( fLength + uLength );
      TCHAR *szNewData = (TCHAR *)malloc( uNewCapacity * sizeof( TCHAR ) );

      lstrcpy( szNewData, fData );
      lstrcpy( &szNewData[fLength], szStr );
      KString kStr;
      kStr.SetData( szNewData, uNewCapacity );
      return std::move(kStr);
    }
    KString operator+( const KString& str )
    {
      return *this + str.Data();
    }
    operator TCHAR*() const
    {
      return this->Data();
    }
    operator std::string() const
    {
      return this->Data();
    }

    // tools
    UINT32 Length() const 
    { 
      if ( fData )
        return lstrlen( fData );
      return 0;
    }
    TCHAR *Data() const
    {
      return fData;
    }
    //KString& Resize( UINT32 uSize );

    ~KString()
    {
      if ( fData )
        free( fData );
    }
  protected:
    UINT32 Caculate( UINT32 uSize )
    {
      UINT32 uNewSize = 0;
      uSize = uSize * 2;
      while ( !(uSize & 0x80000000) )
      {
        uSize <<= 1;
        uNewSize++;
      }
      return  1 << (32 - uNewSize);
    }
    void SetData( TCHAR* str, UINT32 uCapacity )
    {
      assert( !fData && !fLength );
      fData = str;
      fCapacity = uCapacity;
      fLength = lstrlen( str );
    }
  private:
    TCHAR* fData;
    UINT32 fCapacity;
    UINT32 fLength;
  };

};