#include "ashirtdialog.h"
#include <QKeySequence>

#if(QT_VERSION_MAJOR < 6)
#include <QAction>
#endif

AShirtDialog::AShirtDialog( QWidget *parent, bool extraWindowFlags) : QDialog(parent)
{
#if(QT_VERSION_MAJOR > 5)
  addAction(QString(), QKeySequence::Close, this, &AShirtDialog::close);
#else
  closeWindowAction = new QAction(this);
  closeWindowAction->setShortcut(QKeySequence::Close);
  connect(closeWindowAction, &QAction::triggered, this, &AShirtDialog::close);
#endif
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
