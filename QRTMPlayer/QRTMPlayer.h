#pragma once
#include "Header.h"
#include <QtNetwork/QTcpSocket>
#include <QtCore/QUuid>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QWidget>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QMessageBox>
#include <QtCore/QMap>
#include <QtCore/QDebug>
#include <QtCore/QString>
#include "ui_QRTMPlayer.h"
#include "QEncoderThread.h"
#include "QCameraWindow.h"

class QRtmpWindow;
class QRTMPlayer : public QMainWindow
{
    Q_OBJECT

public:
    QRTMPlayer(QWidget *parent = Q_NULLPTR);
    void dealOnline(const QString& peerUUID);
    void dealOffline( const QString& peerUUID );
    void dealRequest( const QString& peerUUID );
    void dealAccept( const QString& peerUUID, const int& timebase );
    void dealRefuse( const QString& peerUUID );
    QString uuidStr() { return fUUID; }
    int timebase() { return fTimebase; }
public slots:
  void dealSendPacket( const Packet& packet );
  void dealReadyRead();
  void dealConfirm();
private:
  Ui::QRTMPlayer ui;
  QTcpSocket* fTcpClient;
  QMap<QString, QToolButton*> fOnlineUsers;
  QMap<QString, QRtmpWindow*> fHashUUIDtoWin;
  QVector<QString> fRtmpWindowArry;
  QString fUUID;
  int fTimebase;
  QEncoderThread *fEncoderThread;
  QCameraWindow *fCameraWindow;
};
