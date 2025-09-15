// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#include "camera_map_view_widget.h"

#include <QtWebEngineWidgets/QWebEngineView>
#include <QtWebChannel/QWebChannel>
#include <QtWidgets/QVBoxLayout>
#include <QtCore/QUrl>
#include <QtCore/QStandardPaths>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QTextStream>

namespace nx::vms::client::desktop {

CameraMapViewWidget::CameraMapViewWidget(QWidget* parent):
    QWidget(parent),
    m_webView(new QWebEngineView(this)),
    m_webChannel(new QWebChannel(this)),
    m_locationSelectionMode(false)
{
    setupUi();
}

CameraMapViewWidget::~CameraMapViewWidget()
{
}

void CameraMapViewWidget::setupUi()
{
    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_webView);

    // Setup web channel for Qt-JavaScript communication
    m_webView->page()->setWebChannel(m_webChannel);
    m_webChannel->registerObject("mapWidget", this);

    connect(m_webView, &QWebEngineView::loadFinished,
        this, &CameraMapViewWidget::onMapReady);

    loadMapHtml();
}

void CameraMapViewWidget::loadMapHtml()
{
    const QString mapHtml = R"(
<!DOCTYPE html>
<html>
<head>
    <title>Camera Location Map</title>
    <meta charset="utf-8" />
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <link rel="stylesheet" href="https://unpkg.com/leaflet@1.9.4/dist/leaflet.css" />
    <style>
        body { margin: 0; padding: 0; }
        #map { height: 100vh; width: 100%; }
    </style>
</head>
<body>
    <div id="map"></div>
    
    <script src="https://unpkg.com/leaflet@1.9.4/dist/leaflet.js"></script>
    <script src="qrc:///qtwebchannel/qwebchannel.js"></script>
    <script>
        let map;
        let cameraMarker;
        let mapWidget;
        let locationSelectionMode = false;

        // Initialize map
        function initMap() {
            map = L.map('map').setView([40.7128, -74.0060], 10); // Default to NYC

            L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png', {
                attribution: '© OpenStreetMap contributors'
            }).addTo(map);

            // Handle map clicks
            map.on('click', function(e) {
                if (locationSelectionMode && mapWidget) {
                    mapWidget.onLocationSelected(e.latlng.lat, e.latlng.lng);
                }
                if (mapWidget) {
                    mapWidget.onMapClicked(e.latlng.lat, e.latlng.lng);
                }
            });
        }

        // Set camera marker
        function setCameraMarker(lat, lng, name) {
            if (cameraMarker) {
                map.removeLayer(cameraMarker);
            }
            
            cameraMarker = L.marker([lat, lng], {
                draggable: true,
                title: name || 'Camera Location'
            }).addTo(map);

            cameraMarker.on('dragend', function(e) {
                const pos = e.target.getLatLng();
                if (mapWidget) {
                    mapWidget.onCameraMarkerMoved(pos.lat, pos.lng);
                }
            });

            map.setView([lat, lng], 15);
        }

        // Clear camera marker
        function clearCameraMarker() {
            if (cameraMarker) {
                map.removeLayer(cameraMarker);
                cameraMarker = null;
            }
        }

        // Center map on location
        function centerOnLocation(lat, lng, zoom) {
            map.setView([lat, lng], zoom || 15);
        }

        // Set location selection mode
        function setLocationSelectionMode(enabled) {
            locationSelectionMode = enabled;
            if (enabled) {
                map.getContainer().style.cursor = 'crosshair';
            } else {
                map.getContainer().style.cursor = '';
            }
        }

        // Initialize when page loads
        document.addEventListener('DOMContentLoaded', function() {
            initMap();
            
            // Setup Qt WebChannel
            new QWebChannel(qt.webChannelTransport, function(channel) {
                mapWidget = channel.objects.mapWidget;
                console.log('WebChannel connected');
            });
        });
    </script>
</body>
</html>
)";

    m_webView->setHtml(mapHtml);
}

void CameraMapViewWidget::onMapReady()
{
    initializeMap();
}

void CameraMapViewWidget::initializeMap()
{
    // Map is ready, can now interact with it
}

void CameraMapViewWidget::setCameraLocation(const std::optional<double>& latitude, 
                                           const std::optional<double>& longitude,
                                           const QString& name)
{
    m_cameraName = name;
    
    if (latitude.has_value() && longitude.has_value())
    {
        QString script = QString("setCameraMarker(%1, %2, '%3');")
            .arg(latitude.value())
            .arg(longitude.value())
            .arg(name);
        m_webView->page()->runJavaScript(script);
    }
    else
    {
        clearCameraLocation();
    }
}

void CameraMapViewWidget::clearCameraLocation()
{
    m_webView->page()->runJavaScript("clearCameraMarker();");
}

void CameraMapViewWidget::centerOnLocation(double latitude, double longitude, int zoom)
{
    QString script = QString("centerOnLocation(%1, %2, %3);")
        .arg(latitude)
        .arg(longitude)
        .arg(zoom);
    m_webView->page()->runJavaScript(script);
}

void CameraMapViewWidget::setLocationSelectionMode(bool enabled)
{
    m_locationSelectionMode = enabled;
    QString script = QString("setLocationSelectionMode(%1);")
        .arg(enabled ? "true" : "false");
    m_webView->page()->runJavaScript(script);
}

bool CameraMapViewWidget::isLocationSelectionMode() const
{
    return m_locationSelectionMode;
}

std::optional<double> CameraMapViewWidget::selectedLatitude() const
{
    return m_selectedLatitude;
}

std::optional<double> CameraMapViewWidget::selectedLongitude() const
{
    return m_selectedLongitude;
}

void CameraMapViewWidget::onLocationSelected(double latitude, double longitude)
{
    m_selectedLatitude = latitude;
    m_selectedLongitude = longitude;
    emit locationSelected(latitude, longitude);
}

void CameraMapViewWidget::onCameraMarkerMoved(double latitude, double longitude)
{
    emit cameraMarkerMoved(latitude, longitude);
}

} // namespace nx::vms::client::desktop
