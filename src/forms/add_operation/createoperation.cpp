#include "createoperation.h"

#include <QRegularExpression>

#include "helpers/netman.h"
#include "helpers/stopreply.h"
#include "dtos/ashirt_error.h"
#include "appsettings.h"

CreateOperation::CreateOperation(QWidget* parent) : QDialog(parent) {
  buildUi();
  wireUi();
}

CreateOperation::~CreateOperation() {
  delete closeWindowAction;
  delete submitButton;
  delete _operationLabel;
  delete responseLabel;
  delete operationNameTextBox;

  delete gridLayout;
  stopReply(&createOpReply);
}

void CreateOperation::show()
{
    QDialog::show(); // display the window
    raise(); // bring to the top (mac)
    activateWindow(); // alternate bring to the top (windows)
}

void CreateOperation::buildUi() {
  gridLayout = new QGridLayout(this);

  submitButton = new LoadingButton(tr("Submit"), this);
  submitButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

  _operationLabel = new QLabel(tr("Operation Name"), this);
  _operationLabel->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
  responseLabel = new QLabel(this);
  operationNameTextBox = new QLineEdit(this);

  // Layout
  /*        0                 1            2
       +---------------+-------------+------------+
    0  | Op Lbl        | [Operation TB]           |
       +---------------+-------------+------------+
    1  | Error Lbl                                |
       +---------------+-------------+------------+
    2  | <None>        | <None>      | Submit Btn |
       +---------------+-------------+------------+
  */

  // row 0
  gridLayout->addWidget(_operationLabel, 0, 0);
  gridLayout->addWidget(operationNameTextBox, 0, 1, 1, 2);

  // row 1
  gridLayout->addWidget(responseLabel, 1, 0, 1, 3);

  // row 2
  gridLayout->addWidget(submitButton, 2, 2);

  closeWindowAction = new QAction(this);
  closeWindowAction->setShortcut(QKeySequence::Close);
  this->addAction(closeWindowAction);

  this->setLayout(gridLayout);
  this->resize(400, 1);
  this->setWindowTitle(tr("Create Operation"));

  Qt::WindowFlags flags = this->windowFlags();
  flags |= Qt::CustomizeWindowHint | Qt::WindowStaysOnTopHint | Qt::WindowMinMaxButtonsHint |
           Qt::WindowCloseButtonHint;
  this->setWindowFlags(flags);
}

void CreateOperation::wireUi() {
  connect(submitButton, &QPushButton::clicked, this, &CreateOperation::submitButtonClicked);
}

void CreateOperation::submitButtonClicked() {
  responseLabel->clear();
  auto name = operationNameTextBox->text().trimmed();
  auto slug = makeSlugFromName(name);

  if (slug.isEmpty()) {
    responseLabel->setText(
        (name.isEmpty())
        ? tr("The Operation Name must not be empty")
        : tr("The Operation Name must include letters or numbers")
    );
    return;
  }

  submitButton->startAnimation();
  submitButton->setEnabled(false);
  createOpReply = NetMan::getInstance().createOperation(name, slug);
  connect(createOpReply, &QNetworkReply::finished, this, &CreateOperation::onRequestComplete);
}

QString CreateOperation::makeSlugFromName(QString name) {
  static QRegularExpression invalidCharsRegex(QStringLiteral("[^A-Za-z0-9]+"));
  static QRegularExpression startOrEndDash(QStringLiteral("^-|-$"));

  return name.toLower().replace(invalidCharsRegex, QStringLiteral("-")).replace(startOrEndDash, QString());
}

void CreateOperation::onRequestComplete() {
  bool isValid;
  auto data = NetMan::extractResponse(createOpReply, isValid);
  if (isValid) {
    dto::Operation op = dto::Operation::parseData(data);
    AppSettings::getInstance().setOperationDetails(op.slug, op.name);
    operationNameTextBox->clear();
    this->close();
  }
  else {
    dto::AShirtError err = dto::AShirtError::parseData(data);
    if (err.error.contains(QStringLiteral("slug already exists"))) {
      responseLabel->setText(tr("A similar operation name already exists. Please try a new name."));
    }
    else {
      responseLabel->setText(tr("Got an unexpected error: %1").arg(err.error));
    }
  }

  submitButton->stopAnimation();
  submitButton->setEnabled(true);

  tidyReply(&createOpReply);
}
