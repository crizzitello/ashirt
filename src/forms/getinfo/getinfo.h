// Copyright 2020, Verizon Media
// Licensed under the terms of MIT. See LICENSE file in project root for terms.

#pragma once

#include <QGridLayout>
#include <QAction>
#include <QNetworkReply>

#include "components/evidence_editor/evidenceeditor.h"
#include "components/loading/qprogressindicator.h"
#include "components/loading_button/loadingbutton.h"
#include "db/databaseconnection.h"
#include "dtos/tag.h"
#include "ashirtdialog/ashirtdialog.h"

namespace Ui {
class GetInfo;
}

class GetInfo : public AShirtDialog {
  Q_OBJECT

 public:
  explicit GetInfo(DatabaseConnection *db, qint64 evidenceID, QWidget *parent = nullptr);
  ~GetInfo();

 private:
  void buildUi();
  void wireUi();
  bool saveData();
  void showEvent(QShowEvent *evt) override;

 signals:
  void setActionButtonsEnabled(bool enabled);
 private slots:
  void submitButtonClicked();
  void deleteButtonClicked();
  void onUploadComplete();

 public:
 signals:
  void evidenceSubmitted(model::Evidence evidence);

 private:
  DatabaseConnection *db;
  qint64 evidenceID;
  QNetworkReply *uploadAssetReply = nullptr;

  // Ui Components
  EvidenceEditor *evidenceEditor = nullptr;
  LoadingButton *submitButton = nullptr;
};
