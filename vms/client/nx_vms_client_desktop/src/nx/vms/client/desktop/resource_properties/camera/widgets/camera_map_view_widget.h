// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#pragma once

#include <QtWidgets/QWidget>
#include <QtCore/QScopedPointer>
#include <optional>

class QWebEngineView;
class QWebChannel;

namespace nx::vms::client::desktop {

/**
 * Widget that displays an interactive OpenStreetMap using Leaflet.js for camera location editing.
 * Supports marker placement, location selection, and coordinate synchronization.
 */
class CameraMapViewWidget: public QWidget
{
    Q_OBJECT

public:
    explicit CameraMapViewWidget(QWidget* parent = nullptr);
    virtual ~CameraMapViewWidget();

    // Camera location management
    void setCameraLocation(const std::optional<double>& latitude, 
                          const std::optional<double>& longitude,
                          const QString& name = QString());
    void clearCameraLocation();
    
    // Map view control
    void centerOnLocation(double latitude, double longitude, int zoom = 15);
    void setLocationSelectionMode(bool enabled);
    bool isLocationSelectionMode() const;

    // Map state
    std::optional<double> selectedLatitude() const;
    std::optional<double> selectedLongitude() const;

signals:
    void locationSelected(double latitude, double longitude);
    void cameraMarkerMoved(double latitude, double longitude);
    void mapClicked(double latitude, double longitude);

public slots:
    void onLocationSelected(double latitude, double longitude);
    void onCameraMarkerMoved(double latitude, double longitude);

private slots:
    void onMapReady();

private:
    void setupUi();
    void initializeMap();
    void loadMapHtml();

private:
    QWebEngineView* m_webView;
    QWebChannel* m_webChannel;
    bool m_locationSelectionMode;
    std::optional<double> m_selectedLatitude;
    std::optional<double> m_selectedLongitude;
    QString m_cameraName;
};

} // namespace nx::vms::client::desktop
