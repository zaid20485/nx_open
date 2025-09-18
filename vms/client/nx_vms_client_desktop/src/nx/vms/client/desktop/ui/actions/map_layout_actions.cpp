// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#include "map_layout_actions.h"

#include <QtGui/QAction>

#include <nx/vms/client/desktop/resource_views/map_layout/map_layout_widget.h>
#include <nx/vms/client/desktop/window_context.h>
#include <nx/vms/client/desktop/workbench/workbench.h>
#include <nx/vms/client/desktop/system_context.h>
#include <core/resource/layout_resource.h>
#include <core/resource_management/resource_pool.h>
#include <ui/workbench/workbench_context.h>
#include <ui/workbench/workbench_layout.h>
#include <ui/workbench/workbench_item.h>

namespace nx::vms::client::desktop {
namespace ui {
namespace actions {

void openMapLayout(WindowContext* windowContext)
{
    if (!windowContext)
        return;
    
    // Create map widget directly as a standalone window
    auto mapWidget = new MapLayoutWidget(windowContext);
    mapWidget->setWindowTitle("Camera Map View");
    mapWidget->setAttribute(Qt::WA_DeleteOnClose);
    mapWidget->resize(1200, 800);
    
    // Load cameras and enable auto-refresh
    mapWidget->loadCamerasFromSystem();
    mapWidget->setAutoRefresh(true, 30000); // Refresh every 30 seconds
    
    // Show the map widget
    mapWidget->show();
}

void registerMapLayoutActions(QWidget* parent, WindowContext* windowContext)
{
    // Create action for opening map layout
    auto openMapAction = new QAction("Open Camera Map", parent);
    openMapAction->setShortcut(QKeySequence("Ctrl+M"));
    openMapAction->setIcon(QIcon(":/skin/buttons/map.svg"));
    openMapAction->setStatusTip("Open camera map view showing all camera locations");
    
    QObject::connect(openMapAction, &QAction::triggered, [windowContext]() {
        openMapLayout(windowContext);
    });
    
    // Add to menu (this would typically be added to the main menu)
    // The actual menu integration would be done in the main window setup
}

} // namespace actions
} // namespace ui
} // namespace nx::vms::client::desktop
