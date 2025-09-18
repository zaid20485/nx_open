// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#pragma once

#include <QtWidgets/QWidget>
#include <QtCore/QScopedPointer>
#include <QtCore/QTimer>

#include <nx/utils/uuid.h>
#include <nx/vms/client/desktop/window_context_aware.h>
#include <core/resource/resource_fwd.h>

class QWebEngineView;
class QWebChannel;
class QToolBar;
class QAction;

namespace nx::vms::client::desktop {

/**
 * Map Layout Widget - A new layout type that displays all cameras with geolocation data
 * on an interactive map. This layout provides a geographical view of the surveillance system.
 */
class MapLayoutWidget: public QWidget, public WindowContextAware
{
    Q_OBJECT

public:
    struct CameraInfo
    {
        nx::Uuid id;
        QString name;
        double latitude;
        double longitude;
        QString status; // "online", "offline", "recording", "unauthorized"
        QString address;
        QnVirtualCameraResourcePtr resource;
    };

    explicit MapLayoutWidget(WindowContext* windowContext, QWidget* parent = nullptr);
    virtual ~MapLayoutWidget();

    // Layout management
    void setLayoutResource(const QnLayoutResourcePtr& layout);
    QnLayoutResourcePtr layoutResource() const;

    // Camera management
    void addCamera(const CameraInfo& camera);
    void updateCamera(const CameraInfo& camera);
    void removeCamera(const nx::Uuid& cameraId);
    void clearAllCameras();
    
    // Bulk operations
    void loadCamerasFromSystem();
    void refreshCameraStatuses();
    
    // Map controls
    void centerOnCamera(const nx::Uuid& cameraId);
    void fitAllCameras();
    void setMapType(const QString& mapType); // "street", "satellite", "hybrid", "terrain"
    void setZoomLevel(int zoom);
    
    // Filtering and grouping
    void setStatusFilter(const QStringList& statuses);
    void setNameFilter(const QString& pattern);
    void setLocationBounds(double minLat, double maxLat, double minLon, double maxLon);
    void groupCamerasByClusters(bool enabled);
    
    // Auto-refresh
    void setAutoRefresh(bool enabled, int intervalMs = 30000);
    bool isAutoRefreshEnabled() const;
    
    // Export/Import
    void exportMapView(const QString& filePath);
    void importCameraLocations(const QString& filePath);

signals:
    void cameraSelected(const nx::Uuid& cameraId);
    void cameraDoubleClicked(const nx::Uuid& cameraId);
    void camerasUpdated(int count);
    void mapTypeChanged(const QString& mapType);
    void zoomLevelChanged(int zoom);

public slots:
    void onCameraResourceAdded(const QnResourcePtr& resource);
    void onCameraResourceRemoved(const QnResourcePtr& resource);
    void onCameraResourceChanged(const QnResourcePtr& resource);
    void onCameraStatusChanged(const QnResourcePtr& resource);
    
private slots:
    void onMapReady();
    void onAutoRefreshTimer();
    void onCameraSelectedInMap(const QString& cameraIdString);
    void onCameraDoubleClickedInMap(const QString& cameraIdString);
    void onMapBoundsChanged(double minLat, double maxLat, double minLon, double maxLon);
    void onMapZoomChanged(int zoom);
    void onToggleFullscreen();
    void onExportView();
    void onImportLocations();
    void onShowSettings();

private:
    void setupUi();
    void setupToolbar();
    void initializeMap();
    void loadMapHtml();
    void connectToResourcePool();
    void updateCameraOnMap(const CameraInfo& camera);
    CameraInfo createCameraInfo(const QnVirtualCameraResourcePtr& camera);
    QString getCameraStatusColor(const QString& status) const;
    QString getCameraStatusIcon(const QString& status) const;
    void applyFilters();
    void updateStatistics();

private:
    struct Private;
    QScopedPointer<Private> d;
};

} // namespace nx::vms::client::desktop
