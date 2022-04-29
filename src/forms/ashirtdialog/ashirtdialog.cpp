#include "ashirtdialog.h"
#include <QKeySequence>

AShirtDialog::AShirtDialog( QWidget *parent, bool extraWindowFlags) : QDialog(parent)
{
  addAction(QString(), QKeySequence::Close, this, &AShirtDialog::close);
  if(extraWindowFlags) {
    Qt::WindowFlags flags = windowFlags();
    flags |= Qt::CustomizeWindowHint | Qt::WindowStaysOnTopHint | Qt::WindowMinMaxButtonsHint |
           Qt::WindowCloseButtonHint;
      setWindowFlags(flags);
  }
}

void AShirtDialog::show()
{
    QDialog::show(); // display the window
    raise(); // bring to the top (mac)
    activateWindow(); // alternate bring to the top (windows)
}
