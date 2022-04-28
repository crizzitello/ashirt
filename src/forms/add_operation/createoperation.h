#pragma once

#include <QAction>
#include <QDialog>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QNetworkReply>

#include "components/loading_button/loadingbutton.h"

class CreateOperation : public QDialog {
  Q_OBJECT

 public:
  explicit CreateOperation(QWidget *parent = nullptr);
  ~CreateOperation();
  void show();

 private:
  void submitButtonClicked();

 private slots:
  void onRequestComplete();
  QString makeSlugFromName(QString name);

 private:

  QNetworkReply* createOpReply = nullptr;

  // ui elements
  LoadingButton* submitButton = nullptr;
  QLabel* responseLabel = nullptr;
  QLineEdit* operationNameTextBox = nullptr;
};
