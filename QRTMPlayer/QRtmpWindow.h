#pragma once
#include "PriorityQueue.h"

#include <QWidget>
#include <QString>
#include <QtWidgets/QLabel>
#include "QDecoderThread.h"
#include <QBoxLayout>


class QRtmpWindow : public QWidget
{
  Q_OBJECT

public:
	QRtmpWindow( const QString& uuid, QWidget *parent = Q_NULLPTR);
	~QRtmpWindow();
  PriorityQueue& PriQueue() { return fPriQue; }
  void SetTimebase( const qint32& timebase ) { fTimebase = timebase; }
  qint32 GetTimebase() { return fTimebase; }
  void SetLost() { fLostConnection = true; }
  bool Lost() { return fLostConnection; }
  void SetYUV( char *data, int size, int cx, int cy );
  void StartDecoder() { fDecoderThread->start(); }
signals:
  void sigUpdateFrame( unsigned char *rgbData, int cx, int cy );
protected slots:
  void dealUpdateFrame( unsigned char *rgbData, int cx, int cy );
private:
  qint32 fTimebase;
  bool fLostConnection;
  qint32 fFrameCounter;
  PriorityQueue fPriQue;
  QString fUUID;
  QLabel* fLabel;
  QDecoderThread* fDecoderThread;
  QHBoxLayout *fMainLayout;
};
