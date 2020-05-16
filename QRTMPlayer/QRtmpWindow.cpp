#include "QRtmpWindow.h"
#include <QImage>

extern "C"
{
#include "libavutil/opt.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/frame.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
};

QRtmpWindow::QRtmpWindow( const QString& uuid, QWidget *parent)
  : QWidget( parent ), 
  fUUID( uuid ), 
  fTimebase( 0 ),
  fLostConnection( false ),
  fDecoderThread( NULL ),
  fFrameCounter( 0 )
{
	//ui.setupUi(this);
  fLabel = new QLabel( this );
  fDecoderThread = new QDecoderThread( this );

  fMainLayout = new QHBoxLayout( this );
  fMainLayout->addWidget( fLabel );
  setLayout( fMainLayout );

  connect( this, SIGNAL( sigUpdateFrame( unsigned char *, int, int ) ), this, 
    SLOT( dealUpdateFrame( unsigned char *, int, int ) ) );
}

QRtmpWindow::~QRtmpWindow()
{
}

void QRtmpWindow::SetYUV( char *yuvData, int size, int cxSour, int cySour )
{
  int cxDst = cxSour / 2; // 原图一半
  int cyDst = cySour / 2; // 原图一半
  AVFrame *pFrame = av_frame_alloc();
  AVFrame *pFrameRGB = av_frame_alloc();
  int numBytes = avpicture_get_size( AV_PIX_FMT_RGB32, cxDst, cyDst );
  uint8_t* rgbBuffer = (uint8_t *)av_malloc( numBytes * sizeof( uint8_t ) );
  avpicture_fill( (AVPicture *)pFrameRGB, rgbBuffer, AV_PIX_FMT_RGB32, cxDst, cyDst );
  //设置图像转换上下文
  
  SwsContext* img_convert_ctx = sws_getContext( cxSour, cySour, AV_PIX_FMT_YUV420P, cxDst, cyDst, 
    AV_PIX_FMT_RGB32, SWS_BICUBIC, nullptr, nullptr, nullptr );

  avpicture_fill( (AVPicture *)pFrame, (uint8_t *)yuvData, AV_PIX_FMT_YUV420P, cxSour, cySour );//这里的长度和高度跟之前保持一致
    //转换图像格式，将解压出来的YUV420P的图像转换为RGB的图像
  sws_scale( img_convert_ctx,
    (uint8_t const * const *)pFrame->data,
    pFrame->linesize, 0, cySour, pFrameRGB->data,
    pFrameRGB->linesize );

  emit sigUpdateFrame( (unsigned char *)pFrameRGB->data[0], cxDst, cyDst );
}

void QRtmpWindow::dealUpdateFrame( unsigned char *rgbData, int cx, int cy )
{
  QImage tmpImg( rgbData, cx, cy, QImage::Format_RGB32 );
  fLabel->setPixmap( QPixmap::fromImage( tmpImg ) );
  fLabel->show();
}

