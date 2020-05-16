#pragma once
#include <qthread.h>

class QRtmpWindow;
class QDecoderThread :
	public QThread
{
public:
  QDecoderThread( QRtmpWindow *rtmpWindow );
  ~QDecoderThread();
  virtual void run();
private:
  QRtmpWindow* fRtmpWindow;
};

