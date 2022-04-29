#pragma once

#include <QDialog>

/**
 * @brief The AShirtDialog class represents a base class used for all dialogs.
 */
class AShirtDialog : public QDialog {
  Q_OBJECT

 public:
  ///Create a AShirtDialog set extraWindowFlags true to set the additional window flags to force always on top.
  AShirtDialog(QWidget* parent = nullptr, bool extraWindowFlags = false);

  /// show Overridden Show forces window to top
  void show();

};
