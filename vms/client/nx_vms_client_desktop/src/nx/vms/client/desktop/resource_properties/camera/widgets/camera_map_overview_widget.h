// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#pragma once

#include <QtWidgets/QWidget>
#include <QtCore/QScopedPointer>
#include <QtCore/QTimer>

#include <nx/utils/uuid.h>

class QWebEngineView;
class QWebChannel;

namespace nx::vms::api { struct CameraData; }

namespace nx::vms::client::desktop {

/**
 * Widget that displays a system-wide overview map showing all cameras with location data.
 * Shows camera status (online/offline/recording) and allows camera selection.
 */
class CameraMapOverviewWidget: public QWidget
{
    Q_OBJECT

public:
    struct CameraLocationInfo
    {
        nx::Uuid id;
        QString name;
        double latitude;
        double longitude;
        QString status; // "online", "offline", "recording", "unauthorized"
        QString address;
    };

    explicit CameraMapOverviewWidget(QWidget* parent = nullptr);
    virtual ~CameraMapOverviewWidget();

    // Camera management
    void addCamera(const CameraLocationInfo& camera);
    void updateCamera(const CameraLocationInfo& camera);
    void removeCamera(const nx::Uuid& cameraId);
    void clearAllCameras();

    // Bulk operations
    void setCameras(const QList<CameraLocationInfo>& cameras);
    void refreshCameras();

    // Map control
    void centerOnCamera(const nx::Uuid& cameraId);
    void fitAllCameras();

    // Auto-refresh
    void setAutoRefresh(bool enabled, int intervalMs = 30000);
    bool isAutoRefreshEnabled() const;

signals:
    void cameraSelected(const nx::Uuid& cameraId);
    void cameraDoubleClicked(const nx::Uuid& cameraId);
    void refreshRequested();

public slots:
    void onCameraSelected(const QString& cameraIdString);
    void onCameraDoubleClicked(const QString& cameraIdString);
    void onRefreshRequested();

private slots:
    void onMapReady();
    void onAutoRefreshTimer();

private:
    void setupUi();
    void initializeMap();
    void loadMapHtml();
    void updateCameraOnMap(const CameraLocationInfo& camera);
    QString getCameraStatusColor(const QString& status) const;
    QString getCameraStatusIcon(const QString& status) const;

private:
    QWebEngineView* m_webView;
    QWebChannel* m_webChannel;
    QTimer* m_autoRefreshTimer;
    QList<CameraLocationInfo> m_cameras;
    bool m_mapReady;
};

} // namespace nx::vms::client::desktop
