#pragma execution_character_set("utf-8")
#include <QtWidgets/QApplication>
#include "QRTMPlayer.h"

int main( int argc, char *argv[] )
{
  QApplication app(argc, argv);
  QRTMPlayer *rtmpPlayer = new QRTMPlayer;
  rtmpPlayer->setWindowTitle( "VVC����ƽ̨" );
  rtmpPlayer->show();
  return app.exec();
}