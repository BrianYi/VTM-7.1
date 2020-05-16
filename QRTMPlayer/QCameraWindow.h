#pragma once

#include <QWidget>
#include "QMsgDlg.h"
#include <QtMultimedia/QCameraImageCapture>
#include <QtMultimediaWidgets/QCameraViewfinder>
#include <QtMultimedia/QCameraInfo>

class QCameraWindow : public QWidget
{
	Q_OBJECT

public:
  QCameraWindow( QWidget *parent);
	~QCameraWindow();
  void start();
  void stop();
  void ShowCameraInfo();
  void UpdateCameraList();

private:
  QCamera*  fCamera;
  QCameraViewfinder* fViewfinder;
  QCameraImageCapture* fImageCapture;
  QList<QCameraInfo> fCameraList;
};
