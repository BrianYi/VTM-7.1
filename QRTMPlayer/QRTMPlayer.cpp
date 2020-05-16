#include "PriorityQueue.h"
#include "QRTMPlayer.h"
#include "Packet.h"
#include "QRtmpWindow.h"
#include "QMsgDlg.h"

QRTMPlayer::QRTMPlayer(QWidget *parent)
    : QMainWindow(parent)
{
  ui.setupUi( this );
  fTcpClient = new QTcpSocket( this );
  fTcpClient->abort();
  fTcpClient->connectToHost( SERVER_IP, SERVER_PORT );
  
  if ( !fTcpClient->waitForConnected( 1000 ) )
    QMessageBox::information( parent, "INFO", "failed to connect to server!" );
  
  fEncoderThread = new QEncoderThread( this );
  fCameraWindow = new QCameraWindow( this );
  fCameraWindow->hide();
  ui.hLayout->addWidget( fCameraWindow, 1 );

  fUUID = QUuid::createUuid().toString();
  fTimebase = 1000 / 25;

  this->dealOnline( fUUID );

  bool bRet = connect( fTcpClient, SIGNAL( readyRead() ), this, SLOT( dealReadyRead() ) );
  assert( bRet );
  bRet = connect( fEncoderThread, SIGNAL( sigSendPacket( const Packet& ) ), this, SLOT( dealSendPacket( const Packet& ) ) );
  assert( bRet );
  qRegisterMetaType<Packet>( "Packet" );
  //bRet = connect( this, SIGNAL( sigSendPacket( Packet& ) ), this, SLOT( dealSendPacket( Packet& ) ) );
  //assert( bRet );

  this->dealSendPacket( PacketUtils::new_session_packet( get_timestamp_ms(), fUUID.toStdString() ) );
}

void QRTMPlayer::dealOnline( const QString& peerUUID )
{
  QToolButton *toolButton = new QToolButton;
  toolButton->setToolButtonStyle( Qt::ToolButtonTextUnderIcon );
  toolButton->setIcon( QIcon( ":/images/splash_icon_wechat_normal@2x.png" ) );
  toolButton->setIconSize( QSize(92, 92) );
  toolButton->setText( peerUUID );
  assert( fOnlineUsers.count( peerUUID ) == 0 );
  fOnlineUsers[peerUUID] = toolButton;
  ui.hLayout->addWidget( toolButton );

  connect( toolButton, SIGNAL( clicked() ), this, SLOT( dealConfirm() ) );
}

void QRTMPlayer::dealOffline( const QString& peerUUID )
{
  if ( fOnlineUsers.count( peerUUID ) )
  {
    QToolButton *toolButton = fOnlineUsers[peerUUID];
    ui.hLayout->removeWidget( toolButton );
    delete toolButton;
  }
}

void QRTMPlayer::dealRequest( const QString& peerUUID )
{
  if ( QMsgDlg::MSGDLG( this, QString( "用户%1想要与您进行视频会话?" ).arg( peerUUID ), true ) == QMsgDlg::Accepted )
  {
    this->dealSendPacket( PacketUtils::accept_packet( peerUUID.length(), fUUID.toStdString(), 
      fTimebase, peerUUID.toStdString().c_str() ) );
  }
  else
  {
    this->dealSendPacket( PacketUtils::refuse_packet( peerUUID.length(), fUUID.toStdString(),
      fTimebase, peerUUID.toStdString().c_str() ) );
    return;
  }

  for (auto it : fOnlineUsers)
    it->hide();

  QRtmpWindow *rtmpWindow = fHashUUIDtoWin[peerUUID];
  rtmpWindow->SetTimebase( fTimebase );
  rtmpWindow->show();
  fRtmpWindowArry.push_back( peerUUID );

  ui.hLayout->addWidget( rtmpWindow, 1 );

  if ( !fEncoderThread->isRunning() )
  {
    fCameraWindow->show();
    fCameraWindow->start();

    rtmpWindow->StartDecoder();
    fEncoderThread->start();
  }
}

void QRTMPlayer::dealAccept( const QString& peerUUID, const qint32& timebase )
{
  for ( auto it : fOnlineUsers )
    it->hide();

  QRtmpWindow *rtmpWindow = fHashUUIDtoWin[peerUUID];
  rtmpWindow->SetTimebase( timebase );
  rtmpWindow->show();
  fRtmpWindowArry.push_back( peerUUID );

  ui.hLayout->addWidget( rtmpWindow, 1 );


  if ( !fEncoderThread->isRunning() )
  {
    fCameraWindow->show();
    fCameraWindow->start();

    rtmpWindow->StartDecoder();
    fEncoderThread->start();
  }
}

void QRTMPlayer::dealRefuse( const QString& peerUUID )
{
  QMsgDlg::MSGDLG( this, QString( "用户%1拒绝了您的请求." ).arg( peerUUID ), true );
}

void QRTMPlayer::dealSendPacket( const Packet& pkt )
{
  Packet packet( pkt );
  PACKET rawNetPacket = packet.raw_net_packet();
  qint64 writenSize = 0;

  writenSize = fTcpClient->write( (char *)&rawNetPacket, MAX_PACKET_SIZE );
  while ( fTcpClient->bytesToWrite())
  {
    if ( fTcpClient->waitForBytesWritten() )
      fTcpClient->flush();
  }
}

void QRTMPlayer::dealReadyRead()
{
  char buf[MAX_PACKET_SIZE];
  while ( !fTcpClient->atEnd() )
  {
    fTcpClient->read( buf, MAX_PACKET_SIZE );
    Packet* ptrPacket = new Packet( buf );
    QString peerUUID = QString::fromStdString(ptrPacket->app());
    switch ( ptrPacket->type() )
    {
      case Push:
      case Fin:
      {
        if ( fHashUUIDtoWin.count( peerUUID ) )
        {
          QRtmpWindow *rtmpWindow = (QRtmpWindow *)fHashUUIDtoWin[peerUUID];
          PriorityQueue& priQue = rtmpWindow->PriQueue();
          priQue.push( PacketUtils::new_packet( *ptrPacket ) );
        }
        break;
      }
      case Pull:
        break;
      case Ack:
        break;
      case Err:
        break;
      case OnlineSessions:
      {
        QString str = ptrPacket->body();
        str.resize( ptrPacket->size() );
        qDebug() << "OnlineSessions from server: " << buf;
        QStringList appArry = str.split( "\n", QString::SkipEmptyParts);
        for ( int i = 0; i < appArry.size(); ++i )
        {
          // new session
          QString peerUUID = appArry[i];
          QRtmpWindow *rtmpWindow = new QRtmpWindow(peerUUID);
          assert( fHashUUIDtoWin.count( peerUUID ) == 0);
          fHashUUIDtoWin[peerUUID] = rtmpWindow;
          this->dealOnline( peerUUID );
        }
        break;
      }
      case NewSession:
      {
        qDebug() << "NewSession from " << ptrPacket->app().c_str();
        // new session
        QRtmpWindow *rtmpWindow = new QRtmpWindow( peerUUID );
        fHashUUIDtoWin[peerUUID] = rtmpWindow;
        this->dealOnline( peerUUID );
        break;
      }
      case LostSession:
      {
        qDebug() << "LostSession from " << ptrPacket->app().c_str();
        this->dealOffline( peerUUID );
        break;
      }
      case BuildConnect:
      {
        qDebug() << "BuildConnect from " << ptrPacket->app().c_str();
        assert( fHashUUIDtoWin.count( peerUUID ) );
        this->dealRequest( peerUUID );
        break;
      }
      case Accept:
      {
        qDebug() << "Accept from " << ptrPacket->app().c_str();
        assert( fHashUUIDtoWin.count( peerUUID ) );
        this->dealAccept( peerUUID, ptrPacket->timebase() );

        break;
      }
      case Refuse:
      {
        qDebug() << "Refused from " << ptrPacket->app().c_str();
        assert( fHashUUIDtoWin.count( peerUUID ) );
        this->dealRefuse( peerUUID );
        break;
      }
      default:

        break;
    }
  }
}

void QRTMPlayer::dealConfirm()
{
  QToolButton *toolButton = (QToolButton *)sender();
  QString peerUUID = toolButton->text();
  if ( peerUUID == fUUID )
    return;

  if ( QMsgDlg::MSGDLG( this, QString( "您确定要与用户%1进行视频会话?" ).arg( peerUUID ), true ) == QMsgDlg::Accepted )
  {
    this->dealSendPacket( PacketUtils::build_connect_packet( peerUUID.length(),
      fUUID.toStdString(), peerUUID.toStdString().c_str() ) );
  }
}
