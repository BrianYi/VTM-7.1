#pragma once
#include <CommCtrl.h>

namespace K 
{
  typedef struct tagKTBBUTTON
  {
    const TCHAR* szText;
    const TCHAR* szImageFile;
    int idCommand;
    BYTE fsState;
    BYTE fsStyle;
    DWORD_PTR dwData;
  } KTBBUTTON;

  class KToolbar :
    public KObject
  {
    enum State
    {
      Normal,
      Hot,
      Disabled,
      Num
    };
  public:
    KToolbar():
      KObject()
    {
    }
    virtual ~KToolbar()
    {}
    virtual const TCHAR* GetClassName() 
    {
      return "S_" TOOLBARCLASSNAME; 
    }
//    virtual LRESULT WndProc( UINT uMsg, WPARAM wParam, LPARAM lParam );

    HWND CreateEx( HWND hwndParent, const TCHAR* pstrWindowName, DWORD dwStyle,
      DWORD dwExStyle, int cxImage, int cyImage, 
      int x = CW_USEDEFAULT, int y = CW_USEDEFAULT,
      int cx = CW_USEDEFAULT, int cy = CW_USEDEFAULT, 
      HMENU hMenu = NULL );

    BOOL AddButton( KTBBUTTON* pTBButton, const int iButtonCx = 0, const int iButtonCy = 0 );
    BOOL AddButtons( KTBBUTTON *tbButtons, const INT32& uNum, const int iButtonCx = 0, const int iButtonCy = 0 );
    BOOL DeleteButton( const int& iCommandId );
    INT32 GetButtonCount() { return (INT32)SendMessage( TB_BUTTONCOUNT, 0, 0 ); }
    BOOL SetButtonSize( const SIZE& szButton ) { return SetButtonSize( szButton.cx, szButton.cy ); }
    BOOL SetButtonSize( const int& cxButton, const int& cyButton );
    SIZE GetButtonSize();
    DWORD SetPadding( const INT8& cxPadding, const INT8& cyPadding )
    { return (DWORD)SendMessage( TB_SETPADDING, 0, MAKEWORD( cxPadding, cyPadding ) ); }
    int AddButtonImage( const int& iCommandId, const TCHAR* szImageFile );
    int SetButtonImage( const int& iCommandId, const TCHAR* szImageFile );
    int SetButtonImage( const int& iCommandId, const int& iImageIndex );
    int SetButtonText( const int& iCommandId, const TCHAR* szText );
    std::vector<int> SetButtonImageList( const int& iCommandId, const TCHAR* szImageFile[], const int& iNum );
    int GetButtonImageIndex( const int& iCommandId );
    BOOL HideButton( const int& iCommandId );
    BOOL ShowButton( const int& iCommandId );
  private:
    int _GetIndex( const int& iCommandId, const int& iImageIndex );
    HIMAGELIST fImageList;
    //std::unordered_map<int, TBBUTTON> fCommToTButton;
    std::unordered_map<std::string, int> fImageToIdx;
    std::unordered_map<int, std::vector<int>> fComIDtoImageIndics;
  };

};