#include "KHeader.h"

namespace K 
{
  HWND KToolbar::CreateEx( HWND hwndParent, const TCHAR* pstrWindowName,
    DWORD dwStyle, DWORD dwExStyle, 
    int cxImage, int cyImage,
    int x /*= CW_USEDEFAULT*/, int y /*= CW_USEDEFAULT*/, 
    int cx /*= CW_USEDEFAULT*/, int cy /*= CW_USEDEFAULT*/, 
    HMENU hMenu /*= NULL */ )
  {
    ((KObject *)this)->CreateEx( hwndParent, pstrWindowName, dwStyle, dwExStyle,
      x, y, cx, cy, hMenu );

     fImageList = ImageList_Create( cxImage, cyImage, ILC_COLOR32 | ILC_MASK, 5, 1 );
     SendMessage( TB_SETIMAGELIST, (WPARAM)0, (LPARAM)fImageList );

    return GetWnd();
  }

  BOOL KToolbar::AddButton( KTBBUTTON* pTBButton, const int iButtonCx/* = 0*/, const int iButtonCy/* = 0*/  )
  {
    return AddButtons( pTBButton, 1, iButtonCx, iButtonCy );
  }

  BOOL KToolbar::AddButtons( KTBBUTTON *tbButtons, const INT32& uNum, const int iButtonCx/* = 0*/, const int iButtonCy/* = 0*/ )
  {
    int cxButton = iButtonCx;
    int cyButton = iButtonCy;
    int cxExtra = 0;
    LRESULT bSuccess = FALSE;
    if ( !fImageList )
    {
      return FALSE;
    }
    TBBUTTON *tbButts = (TBBUTTON *)calloc( uNum, sizeof( TBBUTTON ) );
    HBITMAP hBitmap = NULL;
    for (INT32 i = 0; i < uNum; ++i)
    {
      if ( tbButtons[i].szImageFile )
      {
          hBitmap = KUtils::LoadImage( tbButtons[i].szImageFile );
          tbButts[i].iBitmap = ImageList_Add( fImageList, hBitmap, NULL );
          assert( tbButts[i].iBitmap != -1 );
          fImageToIdx[tbButtons[i].szImageFile] = tbButts[i].iBitmap;
          fComIDtoImageIndics[tbButtons[i].idCommand].push_back( tbButts[i].iBitmap );
      }
      tbButts[i].iString = (INT_PTR)tbButtons[i].szText;
      tbButts[i].idCommand = tbButtons[i].idCommand;
      tbButts[i].fsState = tbButtons[i].fsState;
      tbButts[i].fsStyle = tbButtons[i].fsStyle;
      tbButts[i].dwData = tbButtons[i].dwData;
      if ( tbButts[i].fsStyle & BTNS_DROPDOWN )
        cxExtra += 20;
    }

    (void)SendMessage( TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof( TBBUTTON ), 0 );
    bSuccess = SendMessage( TB_ADDBUTTONS, uNum, (LPARAM)tbButts );
    assert( bSuccess );

    SIZE szButton = GetButtonSize();
    if ( !cxButton )
      cxButton = szButton.cx;
    if ( !cyButton )
      cyButton = szButton.cy;

    bSuccess = SetButtonSize( cxButton, cyButton );
    assert( bSuccess );

    SetWindowPos( HWND_TOP, 0, 0, GetButtonCount() * cxButton + cxExtra, cyButton, SWP_NOMOVE );
    InvalidateRect( NULL, TRUE );
    return TRUE;
  }

  BOOL KToolbar::DeleteButton( const int& idCommand )
  {
    int iButtonIndex = (int)SendMessage( TB_COMMANDTOINDEX, idCommand, 0 );
    assert( iButtonIndex != -1 );
    BOOL bSuccess = (BOOL)SendMessage( TB_DELETEBUTTON, iButtonIndex, 0 );
    assert( bSuccess );

    SIZE szButton = GetButtonSize();
    int cxButton = szButton.cx;
    int cyButton = szButton.cy;
    int cxExtra = 0;

    TBBUTTONINFO tbinfo;
    INT32 iButtonCount = GetButtonCount();
    for ( INT32 i = 0; i < iButtonCount; ++i )
    {
      tbinfo.cbSize = sizeof( TBBUTTONINFO );
      tbinfo.dwMask = TBIF_BYINDEX | TBIF_STYLE;
      SendMessage( TB_GETBUTTONINFO, i, (LPARAM)&tbinfo );
      if ( tbinfo.fsStyle & BTNS_DROPDOWN )
        cxExtra += 20;
    }

    SetWindowPos( HWND_TOP, 0, 0, GetButtonCount() * cxButton + cxExtra, cyButton, SWP_NOMOVE );
    InvalidateRect( NULL, TRUE );
    return TRUE;
  }

  BOOL KToolbar::SetButtonSize( const int& cxButton, const int& cyButton )
  {
    return (BOOL)SendMessage( TB_SETBUTTONSIZE, 0, MAKELPARAM( cxButton, cyButton ) );
  }

  SIZE KToolbar::GetButtonSize()
  {
    SIZE szButton;
    DWORD dwButtonSize = (DWORD)SendMessage( TB_GETBUTTONSIZE, 0, 0 );
    szButton.cx = LOWORD( dwButtonSize );
    szButton.cy = HIWORD( dwButtonSize );
    return szButton;
  }

  int KToolbar::AddButtonImage( const int& iCommandId, const TCHAR* szImageFile )
  {
    /*
     * 按钮是第几个无关紧要,通过iCommandID都可以得到
     * 返回图片索引
     */
    int imageIndex = 0;
    if ( !fImageToIdx.count( szImageFile ) )
    {
      HBITMAP hBitmap = KUtils::LoadImage( szImageFile );
      imageIndex = ImageList_Add( fImageList, hBitmap, NULL );
      assert( imageIndex != -1 );
      fImageToIdx[szImageFile] = imageIndex;
      fComIDtoImageIndics[iCommandId].push_back( imageIndex );
    }
    else
      imageIndex = fImageToIdx[szImageFile];
    return _GetIndex( iCommandId, imageIndex );
  }

//   int KToolbar::SetButtonImage( const UINT64& uUUID, const TCHAR* szImageFile )
//   {
//     if ( !fImageToIdx.count( szImageFile ) )
//     {
//       HBITMAP hBitmap = KUtils::LoadImage( szImageFile );
//       int index = ImageList_Add( fImageList, hBitmap, NULL );
//       assert( index != -1 );
//       fImageToIdx[szImageFile] = index;
//     }
//     int index = fCommToTButton[uUUID].iBitmap;
//     TBBUTTONINFO tbinfo;
//     tbinfo.cbSize = sizeof( TBBUTTONINFO );
//     tbinfo.dwMask = TBIF_BYINDEX | TBIF_IMAGE;
//     SendMessage( TB_GETBUTTONINFO, index, (LPARAM)&tbinfo );
//     tbinfo.iImage = fImageToIdx[szImageFile];
//     SendMessage( TB_SETBUTTONINFO, index, (LPARAM)&tbinfo );
//     return index;
//   }

  int KToolbar::SetButtonImage( const int& iCommandId, const TCHAR* szImageFile )
  {
    int index = AddButtonImage( iCommandId, szImageFile );
    int iButtonIndex = (int)SendMessage( TB_COMMANDTOINDEX, iCommandId, 0 );
    assert( iButtonIndex != -1 );
    TBBUTTONINFO tbinfo;
    tbinfo.cbSize = sizeof( TBBUTTONINFO );
    tbinfo.dwMask = TBIF_BYINDEX | TBIF_IMAGE;
    SendMessage( TB_GETBUTTONINFO, iButtonIndex, (LPARAM)&tbinfo );
    tbinfo.iImage = fComIDtoImageIndics[iCommandId][index];
    SendMessage( TB_SETBUTTONINFO, iButtonIndex, (LPARAM)&tbinfo );
    return iButtonIndex;
  }

  int KToolbar::SetButtonImage( const int& iCommandId, const int& iIndex )
  {
    int iButtonIndex = (int)SendMessage( TB_COMMANDTOINDEX, iCommandId, 0 );
    assert( iButtonIndex != -1 );
    TBBUTTONINFO tbinfo;
    tbinfo.cbSize = sizeof( TBBUTTONINFO );
    tbinfo.dwMask = TBIF_BYINDEX | TBIF_IMAGE;
    SendMessage( TB_GETBUTTONINFO, iButtonIndex, (LPARAM)&tbinfo );
    tbinfo.iImage = fComIDtoImageIndics[iCommandId][iIndex];
    assert( tbinfo.iImage != -1 );
    SendMessage( TB_SETBUTTONINFO, iButtonIndex, (LPARAM)&tbinfo );
    return iButtonIndex;
  }

  int KToolbar::SetButtonText( const int& iCommandId, const TCHAR* szText )
  {
    int iButtonIndex = (int)SendMessage( TB_COMMANDTOINDEX, iCommandId, 0 );
    assert( iButtonIndex != -1 );
    TBBUTTONINFO tbinfo;
    tbinfo.cbSize = sizeof( TBBUTTONINFO );
    tbinfo.dwMask = TBIF_BYINDEX | TBIF_TEXT;
    SendMessage( TB_GETBUTTONINFO, iButtonIndex, (LPARAM)&tbinfo );
    tbinfo.pszText = (LPTSTR)szText;
    assert( tbinfo.iImage != -1 );
    SendMessage( TB_SETBUTTONINFO, iButtonIndex, (LPARAM)&tbinfo );
    return iButtonIndex;
  }

  std::vector<int> KToolbar::SetButtonImageList( const int& iCommandId, const TCHAR* szImageFile[], const int& iNum )
  {
    std::vector<int> imageIndexArry;
    int iImageIndex;
    for ( int i = 0; i < iNum; ++i )
    {
      iImageIndex = SetButtonImage( iCommandId, szImageFile[i] );
      assert( iImageIndex != -1 );
      imageIndexArry.push_back( iImageIndex );
    }
    return imageIndexArry;
  }

  int KToolbar::GetButtonImageIndex( const int& iCommandId )
  {
    int iButtonIndex = (int)SendMessage( TB_COMMANDTOINDEX, iCommandId, 0 );
    assert( iButtonIndex != -1 );
    TBBUTTONINFO tbinfo;
    tbinfo.cbSize = sizeof( TBBUTTONINFO );
    tbinfo.dwMask = TBIF_BYINDEX | TBIF_IMAGE;
    SendMessage( TB_GETBUTTONINFO, iButtonIndex, (LPARAM)&tbinfo );
    return _GetIndex(iCommandId, tbinfo.iImage);
  }

  BOOL KToolbar::HideButton( const int& iCommandId )
  {
    return (BOOL)SendMessage( TB_HIDEBUTTON, iCommandId, TRUE );
  }

  BOOL KToolbar::ShowButton( const int& iCommandId )
  {
    return (BOOL)SendMessage( TB_HIDEBUTTON, iCommandId, FALSE );
  }

  int KToolbar::_GetIndex( const int& iCommandId, const int& iImageIndex )
  {
    auto& arry = fComIDtoImageIndics[iCommandId];
    for ( int i = 0; i < arry.size(); ++i )
    {
      if ( arry[i] == iImageIndex )
        return i;
    }
    return -1;
  }

};