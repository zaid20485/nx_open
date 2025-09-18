// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#pragma once

#include <QtCore/QObject>

class QWidget;

namespace nx::vms::client::desktop {

class WindowContext;

namespace ui {
namespace actions {

/**
 * Opens a new map layout showing all cameras with geolocation data.
 * @param windowContext The window context to create the layout in
 */
void openMapLayout(WindowContext* windowContext);

/**
 * Registers map layout actions with the application menu system.
 * @param parent Parent widget for the actions
 * @param windowContext The window context for action execution
 */
void registerMapLayoutActions(QWidget* parent, WindowContext* windowContext);

} // namespace actions
} // namespace ui
} // namespace nx::vms::client::desktop
