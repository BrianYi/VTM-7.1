#include "QCameraWindow.h"
#include <QList>

QCameraWindow::QCameraWindow( QWidget *parent)
	: QWidget(parent)
{
  UpdateCameraList();

  fCamera = new QCamera( fCameraList[0], this );
  fViewfinder = new QCameraViewfinder( this );
  fImageCapture = new QCameraImageCapture( fCamera );

  QVBoxLayout *mainLayout = new QVBoxLayout( this );
  mainLayout->addWidget( fViewfinder );
  this->setLayout( mainLayout );

  fCamera->setCaptureMode( QCamera::CaptureStillImage );
  fCamera->setViewfinder( fViewfinder );
}

QCameraWindow::~QCameraWindow()
{
}

void QCameraWindow::start()
{
  fCamera->start();
}

void QCameraWindow::stop()
{
  fCamera->stop();
}

void QCameraWindow::ShowCameraInfo()
{
  UpdateCameraList();

  QString str;
  for ( int i = 0; i < fCameraList.size(); ++i )
  {
    QCameraInfo& camInfo = fCameraList[i];
    str += QString::number(i) + ": " + camInfo.deviceName() + "\t" + camInfo.description() +
       "\t" + camInfo.position() + "\r\n";
  }
  QMsgDlg::MSGDLG( this, str, true );
}

void QCameraWindow::UpdateCameraList()
{
  fCameraList = QCameraInfo::availableCameras();
}
