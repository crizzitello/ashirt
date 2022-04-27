// Copyright 2020, Verizon Media
// Licensed under the terms of MIT. See LICENSE file in project root for terms.

#include "hotkeymanager.h"

#include <QString>
#include <iostream>

#include "appconfig.h"
#include "appsettings.h"
#include "helpers/hotkeys/uglobalhotkeys.h"

HotkeyManager::HotkeyManager(QObject * parent): QObject(parent) {
  hotkeyManager = new UGlobalHotkeys(this);
  connect(hotkeyManager, &UGlobalHotkeys::activated, this, &HotkeyManager::hotkeyTriggered);
}

void HotkeyManager::registerKey(const QString& binding, GlobalHotkeyEvent evt) {
  if(binding.isEmpty())
      return;
  hotkeyManager->registerHotkey(binding, size_t(evt));
}

void HotkeyManager::unregisterKey(GlobalHotkeyEvent evt) {
  hotkeyManager->unregisterHotkey(size_t(evt));
}

void HotkeyManager::hotkeyTriggered(size_t hotkeyIndex) {
  if (hotkeyIndex == ACTION_CAPTURE_AREA) {
    Q_EMIT captureAreaHotkeyPressed();
  }
  else if (hotkeyIndex == ACTION_CAPTURE_WINDOW) {
    Q_EMIT captureWindowHotkeyPressed();
  }
  else if (hotkeyIndex == ACTION_CAPTURE_CODEBLOCK) {
    Q_EMIT codeblockHotkeyPressed();
  }
}

void HotkeyManager::disableHotkeys() {
  hotkeyManager->unregisterAllHotkeys();
}

void HotkeyManager::enableHotkeys() {
  updateHotkeys();
}

void HotkeyManager::updateHotkeys() {
  hotkeyManager->unregisterAllHotkeys();
  registerKey(AppConfig::getInstance().screenshotShortcutCombo, ACTION_CAPTURE_AREA);
  registerKey(AppConfig::getInstance().captureWindowShortcut, ACTION_CAPTURE_WINDOW);
  registerKey(AppConfig::getInstance().captureCodeblockShortcut, ACTION_CAPTURE_CODEBLOCK);
}
