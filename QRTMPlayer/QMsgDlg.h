#pragma once

#include <QDialog>
#include "ui_QMsgDlg.h"

class QMsgDlg : public QDialog
{
	Q_OBJECT

public:
	QMsgDlg(QWidget *parent = Q_NULLPTR);
	~QMsgDlg();
  void SetText( const QString& txt );
public:
  static int MSGDLG( QWidget *parent, const QString& text, bool isModel )
  {
    QMsgDlg msgDlg( parent );
    msgDlg.SetText( text );
    msgDlg.setModal( isModel );
    return msgDlg.exec();
  }
private:
	Ui::QMsgDlg ui;
};
