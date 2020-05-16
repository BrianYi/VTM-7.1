#pragma execution_character_set("utf-8")
#include <QtWidgets/QApplication>
#include "QRTMPlayer.h"

int main( int argc, char *argv[] )
{
  QApplication app(argc, argv);
  QRTMPlayer *rtmpPlayer = new QRTMPlayer;
  rtmpPlayer->setWindowTitle( "VVC会议平台" );
  rtmpPlayer->show();
  return app.exec();
}