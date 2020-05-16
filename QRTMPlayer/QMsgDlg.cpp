#include "QMsgDlg.h"

QMsgDlg::QMsgDlg(QWidget *parent)
	: QDialog(parent)
{
	ui.setupUi(this);
  connect( ui.pushOk, SIGNAL( clicked() ), this, SLOT( accept() ) );
  connect( ui.pushCancel, SIGNAL( clicked() ), this, SLOT( reject() ) );
}

QMsgDlg::~QMsgDlg()
{
}

void QMsgDlg::SetText( const QString& txt )
{
  ui.label->setText( txt );
}
