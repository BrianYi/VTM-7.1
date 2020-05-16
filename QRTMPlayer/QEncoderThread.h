#pragma once
#include <QThread>
#include "QRtmpWindow.h"

class QRTMPlayer;
class QEncoderThread : public QThread
{
	Q_OBJECT

public:
	QEncoderThread(QObject *parent);
	~QEncoderThread();
  virtual void run();
signals:
  void sigSendPacket( const Packet& packet );
private:
  QRTMPlayer* fRTMPlayer;
};
