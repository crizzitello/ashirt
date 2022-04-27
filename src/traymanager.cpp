// Copyright 2020, Verizon Media
// Licensed under the terms of MIT. See LICENSE file in project root for terms.

#include "traymanager.h"

#ifndef QT_NO_SYSTEMTRAYICON

#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QCoreApplication>
#include <QMenu>
#include <QMessageBox>
#include <QTimer>
#include <QDesktopServices>
#include <iostream>
#include "appconfig.h"
#include "appsettings.h"
#include "db/databaseconnection.h"
#include "forms/getinfo/getinfo.h"
#include "helpers/clipboard/clipboardhelper.h"
#include "helpers/netman.h"
#include "helpers/screenshot.h"
#include "helpers/constants.h"
#include "hotkeymanager.h"
#include "models/codeblock.h"
#include "porting/system_manifest.h"

#if defined(Q_OS_WIN)
#include <QSettings>
#endif

TrayManager::TrayManager(QWidget * parent, DatabaseConnection* db)
    : QDialog(parent)
    , db(db)
    , screenshotTool(new Screenshot(this))
    , hotkeyManager(new HotkeyManager(this))
    , updateCheckTimer(new QTimer(this))
    , settingsWindow(new Settings(hotkeyManager, this))
    , evidenceManagerWindow(new EvidenceManager(this->db, this))
    , creditsWindow(new Credits(this))
    , importWindow(new PortingDialog(PortingDialog::Import, this->db, this))
    , exportWindow(new PortingDialog(PortingDialog::Export, this->db, this))
    , createOperationWindow(new CreateOperation(this))
    , currentOperationMenuAction(new QAction(QString(), this))
    , chooseOpStatusAction(new QAction(tr("Loading operations..."), this))
    , newOperationAction(new QAction(tr("New Operation"), this))
    , trayIcon(new QSystemTrayIcon(this))

{
  hotkeyManager->updateHotkeys();
  updateCheckTimer->start(MS_IN_DAY); // every day

  buildUi();
  wireUi();

  // delayed so that windows can listen for get all ops signal
  NetMan::getInstance().refreshOperationsList();
  QTimer::singleShot(5000, this, &TrayManager::checkForUpdate);
}

TrayManager::~TrayManager() {
  setVisible(false);
  cleanChooseOpSubmenu();
}

void TrayManager::buildUi() {
  //Disable Actions
  currentOperationMenuAction->setEnabled(false);
  chooseOpStatusAction->setEnabled(false);
  newOperationAction->setEnabled(false);  // only enable when we have an internet connection

  // Build Tray menu
  auto trayIconMenu = new QMenu(this);
  trayIconMenu->addAction(tr("Add Codeblock from Clipboard"), this, &TrayManager::captureCodeblockActionTriggered);
  trayIconMenu->addAction(tr("Capture Screen Area"), this, &TrayManager::captureAreaActionTriggered);
  trayIconMenu->addAction(tr("Capture Window"), this, &TrayManager::captureWindowActionTriggered);
  trayIconMenu->addAction(tr("View Accumulated Evidence"), evidenceManagerWindow, &EvidenceManager::show);
  trayIconMenu->addSeparator();
  trayIconMenu->addAction(currentOperationMenuAction);
  chooseOpSubmenu = trayIconMenu->addMenu(tr("Select Operation"));
  trayIconMenu->addSeparator();
  auto importExportSubmenu = trayIconMenu->addMenu(tr("Import/Export"));
  trayIconMenu->addAction(tr("Settings"), settingsWindow, &Settings::show);
  trayIconMenu->addAction(tr("About"), creditsWindow, &Credits::show);
  trayIconMenu->addAction(tr("Quit"), qApp, &QCoreApplication::quit);

  // Operations Submenu
  chooseOpSubmenu->addAction(chooseOpStatusAction);
  chooseOpSubmenu->addAction(newOperationAction);
  chooseOpSubmenu->addSeparator();

  // settings submenu
  importExportSubmenu->addAction(tr("Export Data"), exportWindow, &PortingDialog::show);
  importExportSubmenu->addAction(tr("Import Data"), importWindow, &PortingDialog::show);

  setActiveOperationLabel();

#if defined(Q_OS_LINUX)
  QIcon icon = QIcon(palette().text().color().value() >= QColor(Qt::lightGray).value()
                     ? QStringLiteral(":/icons/shirt-light.svg")
                     : QStringLiteral(":/icons/shirt-dark.svg"));
#elif defined(Q_OS_MACOS)
  QIcon icon = QIcon(QStringLiteral(":/icons/shirt-dark.svg"));
  icon.setIsMask(true);
#elif defined(Q_OS_WIN)
  QSettings settings(QStringLiteral("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize"), QSettings::NativeFormat);
  QIcon icon = QIcon(settings.value(QStringLiteral("SystemUsesLightTheme")).toInt() == 0
                     ? QStringLiteral(":/icons/shirt-light.svg")
                     : QStringLiteral(":/icons/shirt-dark.svg"));
#endif

  trayIcon->setContextMenu(trayIconMenu);
  trayIcon->setIcon(icon);
  trayIcon->show();
}

void TrayManager::wireUi() {
  connect(newOperationAction, &QAction::triggered, createOperationWindow, &CreateOperation::show);

  connect(exportWindow, &PortingDialog::portCompleted, this, [this](const QString& path) {
    openServicesPath = path;
    setTrayMessage(OPEN_PATH, tr("Export Complete"), tr("Export saved to: %1\nClick to view").arg(path));
  });
  connect(importWindow, &PortingDialog::portCompleted, this, [this](const QString& path) {
    setTrayMessage(NO_ACTION, tr("Import Complete"), tr("Import retrieved from: %1").arg(path));
  });

  connect(screenshotTool, &Screenshot::onScreenshotCaptured, this,
          &TrayManager::onScreenshotCaptured);

  // connect to hotkey signals
  connect(hotkeyManager, &HotkeyManager::codeblockHotkeyPressed, this,
          &TrayManager::captureCodeblockActionTriggered);
  connect(hotkeyManager, &HotkeyManager::captureAreaHotkeyPressed, this,
          &TrayManager::captureAreaActionTriggered);
  connect(hotkeyManager, &HotkeyManager::captureWindowHotkeyPressed, this,
          &TrayManager::captureWindowActionTriggered);

  // connect to network signals
  connect(&NetMan::getInstance(), &NetMan::operationListUpdated, this,
          &TrayManager::onOperationListUpdated);
  connect(&NetMan::getInstance(), &NetMan::releasesChecked, this, &TrayManager::onReleaseCheck);
  connect(&AppSettings::getInstance(), &AppSettings::onOperationUpdated, this,
          &TrayManager::setActiveOperationLabel);
  
  connect(trayIcon, &QSystemTrayIcon::messageClicked, this, &TrayManager::onTrayMessageClicked);
  connect(trayIcon, &QSystemTrayIcon::activated, this, [this] {
    chooseOpStatusAction->setText(tr("Loading operations..."));
    newOperationAction->setEnabled(false);
    NetMan::getInstance().refreshOperationsList();
  });

  connect(updateCheckTimer, &QTimer::timeout, this, &TrayManager::checkForUpdate);
}

void TrayManager::cleanChooseOpSubmenu() {
  // delete all of the existing events
  for (QAction* act : allOperationActions) {
    chooseOpSubmenu->removeAction(act);
    act->deleteLater();
  }
  allOperationActions.clear();
  selectedAction = nullptr; // clear the selected action to ensure no funny business
}

void TrayManager::closeEvent(QCloseEvent* event) {
#ifdef Q_OS_MACOS
  if (!event->spontaneous() || !isVisible()) {
    return;
  }
#endif
  if (trayIcon->isVisible()) {
    hide();
    event->ignore();
  }
}

void TrayManager::spawnGetInfoWindow(qint64 evidenceID) {
  auto getInfoWindow = new GetInfo(db, evidenceID, this);
  connect(getInfoWindow, &GetInfo::evidenceSubmitted, [](const model::Evidence& evi){
    AppSettings::getInstance().setLastUsedTags(evi.tags);
  });
  getInfoWindow->show();
}

qint64 TrayManager::createNewEvidence(const QString& filepath, const QString& evidenceType) {
  AppSettings& inst = AppSettings::getInstance();
  auto evidenceID = db->createEvidence(filepath, inst.operationSlug(), evidenceType);
  auto tags = inst.getLastUsedTags();
  if (!tags.empty()) {
    db->setEvidenceTags(tags, evidenceID);
  }
  return evidenceID;
}

void TrayManager::captureWindowActionTriggered() {
  if(AppSettings::getInstance().operationSlug().isEmpty()) {
    showNoOperationSetTrayMessage();
    return;
  }
  screenshotTool->captureWindow();
}

void TrayManager::captureAreaActionTriggered() {
  if(AppSettings::getInstance().operationSlug().isEmpty()) {
    showNoOperationSetTrayMessage();
    return;
  }
  screenshotTool->captureArea();
}

void TrayManager::captureCodeblockActionTriggered() {
  if(AppSettings::getInstance().operationSlug().isEmpty()) {
    showNoOperationSetTrayMessage();
    return;
  }
  onCodeblockCapture();
}

void TrayManager::onCodeblockCapture() {
  QString clipboardContent = ClipboardHelper::readPlaintext();
  if (!clipboardContent.isEmpty()) {
    Codeblock evidence(clipboardContent);
    Codeblock::saveCodeblock(evidence);
    try {
      auto evidenceID = createNewEvidence(evidence.filePath(), QStringLiteral("codeblock"));
      spawnGetInfoWindow(evidenceID);
    }
    catch (QSqlError& e) {
      std::cout << "could not write to the database: " << e.text().toStdString() << std::endl;
    }
  }
}

void TrayManager::onScreenshotCaptured(const QString& path) {
  try {
    auto evidenceID = createNewEvidence(path, QStringLiteral("image"));
    spawnGetInfoWindow(evidenceID);
  }
  catch (QSqlError& e) {
    std::cout << "could not write to the database: " << e.text().toStdString() << std::endl;
  }
}

void TrayManager::showNoOperationSetTrayMessage() {
  setTrayMessage(NO_ACTION, tr("Unable to Record Evidence"),
                        tr("No Operation has been selected. Please select an operation first."),
                        QSystemTrayIcon::Warning);
}

void TrayManager::setActiveOperationLabel() {
  const auto& opName = AppSettings::getInstance().operationName();
  currentOperationMenuAction->setText(tr("Operation: %1").arg(opName.isEmpty() ? tr("<None>") : opName));
}

void TrayManager::onOperationListUpdated(bool success,
                                         const std::vector<dto::Operation>& operations) {
  auto currentOp = AppSettings::getInstance().operationSlug();

  if (success) {
    chooseOpStatusAction->setText(tr("Operations loaded"));
    newOperationAction->setEnabled(true);
    cleanChooseOpSubmenu();

    for (const auto& op : operations) {
      auto newAction = new QAction(op.name, chooseOpSubmenu);

      if (currentOp == op.slug) {
        newAction->setCheckable(true);
        newAction->setChecked(true);
        selectedAction = newAction;
      }

      connect(newAction, &QAction::triggered, this, [this, newAction, op] {
        AppSettings::getInstance().setLastUsedTags(std::vector<model::Tag>{}); // clear last used tags
        AppSettings::getInstance().setOperationDetails(op.slug, op.name);
        if (selectedAction) {
          selectedAction->setChecked(false);
          selectedAction->setCheckable(false);
        }
        newAction->setCheckable(true);
        newAction->setChecked(true);
        selectedAction = newAction;
      });
      allOperationActions.append(newAction);
      chooseOpSubmenu->addAction(newAction);
    }
    if (!selectedAction) {
      AppSettings::getInstance().setOperationDetails(QString(), QString());
    }
  }
  else {
    chooseOpStatusAction->setText(tr("Unable to load operations"));
  }
}

void TrayManager::checkForUpdate() {
  NetMan::getInstance().checkForNewRelease(Constants::releaseOwner(), Constants::releaseRepo());
}

void TrayManager::onReleaseCheck(bool success, const std::vector<dto::GithubRelease>& releases) {
  if (!success) {
    return;  // doesn't matter if this fails -- another request will be made later.
  }

  auto digest = dto::ReleaseDigest::fromReleases(Constants::releaseTag(), releases);

  if (digest.hasUpgrade()) {
    setTrayMessage(UPGRADE, tr("A new version is available!"), tr("Click for more info"));
  }
}

void TrayManager::setTrayMessage(MessageType type, const QString& title, const QString& message,
                                 QSystemTrayIcon::MessageIcon icon, int millisecondsTimeoutHint) {
  trayIcon->showMessage(title, message, icon, millisecondsTimeoutHint);
  this->currentTrayMessage = type;
}

void TrayManager::onTrayMessageClicked() {
  switch(currentTrayMessage) {
    case UPGRADE:
      QDesktopServices::openUrl(Constants::releasePageUrl());
      break;
    case OPEN_PATH:
      QDesktopServices::openUrl(openServicesPath);
    case NO_ACTION:
    default:
      break;
  }
}

#endif
