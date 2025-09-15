// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#include "camera_map_overview_widget.h"

#include <QtWebEngineWidgets/QWebEngineView>
#include <QtWebChannel/QWebChannel>
#include <QtWidgets/QVBoxLayout>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>

namespace nx::vms::client::desktop {

CameraMapOverviewWidget::CameraMapOverviewWidget(QWidget* parent):
    QWidget(parent),
    m_webView(new QWebEngineView(this)),
    m_webChannel(new QWebChannel(this)),
    m_autoRefreshTimer(new QTimer(this)),
    m_mapReady(false)
{
    setupUi();
}

CameraMapOverviewWidget::~CameraMapOverviewWidget()
{
}

void CameraMapOverviewWidget::setupUi()
{
    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_webView);

    // Setup web channel for Qt-JavaScript communication
    m_webView->page()->setWebChannel(m_webChannel);
    m_webChannel->registerObject("mapOverview", this);

    connect(m_webView, &QWebEngineView::loadFinished,
        this, &CameraMapOverviewWidget::onMapReady);

    // Setup auto-refresh timer
    m_autoRefreshTimer->setSingleShot(false);
    connect(m_autoRefreshTimer, &QTimer::timeout,
        this, &CameraMapOverviewWidget::onAutoRefreshTimer);

    loadMapHtml();
}

void CameraMapOverviewWidget::loadMapHtml()
{
    const QString mapHtml = R"(
<!DOCTYPE html>
<html>
<head>
    <title>Camera Overview Map</title>
    <meta charset="utf-8" />
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <link rel="stylesheet" href="https://unpkg.com/leaflet@1.9.4/dist/leaflet.css" />
    <style>
        body { margin: 0; padding: 0; }
        #map { height: 100vh; width: 100%; }
        .camera-popup {
            font-family: Arial, sans-serif;
            min-width: 200px;
        }
        .camera-name {
            font-weight: bold;
            margin-bottom: 5px;
        }
        .camera-status {
            padding: 2px 6px;
            border-radius: 3px;
            color: white;
            font-size: 12px;
            margin-bottom: 5px;
        }
        .status-online { background-color: #28a745; }
        .status-offline { background-color: #dc3545; }
        .status-recording { background-color: #007bff; }
        .status-unauthorized { background-color: #ffc107; color: black; }
        .camera-address {
            font-size: 12px;
            color: #666;
        }
    </style>
</head>
<body>
    <div id="map"></div>
    
    <script src="https://unpkg.com/leaflet@1.9.4/dist/leaflet.js"></script>
    <script src="qrc:///qtwebchannel/qwebchannel.js"></script>
    <script>
        let map;
        let mapOverview;
        let cameraMarkers = {};
        let cameraGroup;

        // Initialize map
        function initMap() {
            map = L.map('map').setView([40.7128, -74.0060], 2); // Default world view

            L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png', {
                attribution: '© OpenStreetMap contributors'
            }).addTo(map);

            // Create layer group for camera markers
            cameraGroup = L.layerGroup().addTo(map);
        }

        // Create custom icon based on camera status
        function createCameraIcon(status) {
            let color = '#666666';
            switch(status) {
                case 'online': color = '#28a745'; break;
                case 'offline': color = '#dc3545'; break;
                case 'recording': color = '#007bff'; break;
                case 'unauthorized': color = '#ffc107'; break;
            }
            
            return L.divIcon({
                className: 'camera-marker',
                html: `<div style="background-color: ${color}; width: 20px; height: 20px; border-radius: 50%; border: 2px solid white; box-shadow: 0 2px 4px rgba(0,0,0,0.3);"></div>`,
                iconSize: [24, 24],
                iconAnchor: [12, 12]
            });
        }

        // Add or update camera marker
        function updateCamera(camera) {
            // Remove existing marker if it exists
            if (cameraMarkers[camera.id]) {
                cameraGroup.removeLayer(cameraMarkers[camera.id]);
            }

            // Create new marker
            const marker = L.marker([camera.latitude, camera.longitude], {
                icon: createCameraIcon(camera.status),
                title: camera.name
            });

            // Create popup content
            const popupContent = `
                <div class="camera-popup">
                    <div class="camera-name">${camera.name}</div>
                    <div class="camera-status status-${camera.status}">${camera.status.toUpperCase()}</div>
                    ${camera.address ? `<div class="camera-address">${camera.address}</div>` : ''}
                </div>
            `;
            marker.bindPopup(popupContent);

            // Handle marker events
            marker.on('click', function() {
                if (mapOverview) {
                    mapOverview.onCameraSelected(camera.id);
                }
            });

            marker.on('dblclick', function() {
                if (mapOverview) {
                    mapOverview.onCameraDoubleClicked(camera.id);
                }
            });

            // Add to group and store reference
            cameraGroup.addLayer(marker);
            cameraMarkers[camera.id] = marker;
        }

        // Remove camera marker
        function removeCamera(cameraId) {
            if (cameraMarkers[cameraId]) {
                cameraGroup.removeLayer(cameraMarkers[cameraId]);
                delete cameraMarkers[cameraId];
            }
        }

        // Clear all cameras
        function clearAllCameras() {
            cameraGroup.clearLayers();
            cameraMarkers = {};
        }

        // Center on specific camera
        function centerOnCamera(cameraId) {
            if (cameraMarkers[cameraId]) {
                const marker = cameraMarkers[cameraId];
                map.setView(marker.getLatLng(), 15);
                marker.openPopup();
            }
        }

        // Fit map to show all cameras
        function fitAllCameras() {
            if (Object.keys(cameraMarkers).length > 0) {
                const group = new L.featureGroup(Object.values(cameraMarkers));
                map.fitBounds(group.getBounds(), { padding: [20, 20] });
            }
        }

        // Set multiple cameras
        function setCameras(cameras) {
            clearAllCameras();
            cameras.forEach(camera => updateCamera(camera));
            if (cameras.length > 0) {
                setTimeout(() => fitAllCameras(), 100);
            }
        }

        // Initialize when page loads
        document.addEventListener('DOMContentLoaded', function() {
            initMap();
            
            // Setup Qt WebChannel
            new QWebChannel(qt.webChannelTransport, function(channel) {
                mapOverview = channel.objects.mapOverview;
                console.log('WebChannel connected');
            });
        });
    </script>
</body>
</html>
)";

    m_webView->setHtml(mapHtml);
}

void CameraMapOverviewWidget::onMapReady()
{
    m_mapReady = true;
    initializeMap();
    
    // Update map with any cameras that were added before map was ready
    if (!m_cameras.isEmpty())
    {
        setCameras(m_cameras);
    }
}

void CameraMapOverviewWidget::initializeMap()
{
    // Map is ready for interaction
}

void CameraMapOverviewWidget::addCamera(const CameraLocationInfo& camera)
{
    // Update internal list
    auto it = std::find_if(m_cameras.begin(), m_cameras.end(),
        [&camera](const CameraLocationInfo& c) { return c.id == camera.id; });
    
    if (it != m_cameras.end())
    {
        *it = camera;
    }
    else
    {
        m_cameras.append(camera);
    }

    // Update map if ready
    if (m_mapReady)
    {
        updateCameraOnMap(camera);
    }
}

void CameraMapOverviewWidget::updateCamera(const CameraLocationInfo& camera)
{
    addCamera(camera); // Same logic as add
}

void CameraMapOverviewWidget::removeCamera(const nx::Uuid& cameraId)
{
    // Remove from internal list
    m_cameras.removeIf([&cameraId](const CameraLocationInfo& c) { 
        return c.id == cameraId; 
    });

    // Remove from map if ready
    if (m_mapReady)
    {
        QString script = QString("removeCamera('%1');").arg(cameraId.toString());
        m_webView->page()->runJavaScript(script);
    }
}

void CameraMapOverviewWidget::clearAllCameras()
{
    m_cameras.clear();
    
    if (m_mapReady)
    {
        m_webView->page()->runJavaScript("clearAllCameras();");
    }
}

void CameraMapOverviewWidget::setCameras(const QList<CameraLocationInfo>& cameras)
{
    m_cameras = cameras;
    
    if (m_mapReady)
    {
        // Convert to JSON and send to map
        QJsonArray cameraArray;
        for (const auto& camera : cameras)
        {
            QJsonObject cameraObj;
            cameraObj["id"] = camera.id.toString();
            cameraObj["name"] = camera.name;
            cameraObj["latitude"] = camera.latitude;
            cameraObj["longitude"] = camera.longitude;
            cameraObj["status"] = camera.status;
            cameraObj["address"] = camera.address;
            cameraArray.append(cameraObj);
        }
        
        QJsonDocument doc(cameraArray);
        QString script = QString("setCameras(%1);").arg(QString::fromUtf8(doc.toJson(QJsonDocument::Compact)));
        m_webView->page()->runJavaScript(script);
    }
}

void CameraMapOverviewWidget::refreshCameras()
{
    emit refreshRequested();
}

void CameraMapOverviewWidget::centerOnCamera(const nx::Uuid& cameraId)
{
    if (m_mapReady)
    {
        QString script = QString("centerOnCamera('%1');").arg(cameraId.toString());
        m_webView->page()->runJavaScript(script);
    }
}

void CameraMapOverviewWidget::fitAllCameras()
{
    if (m_mapReady)
    {
        m_webView->page()->runJavaScript("fitAllCameras();");
    }
}

void CameraMapOverviewWidget::setAutoRefresh(bool enabled, int intervalMs)
{
    if (enabled)
    {
        m_autoRefreshTimer->start(intervalMs);
    }
    else
    {
        m_autoRefreshTimer->stop();
    }
}

bool CameraMapOverviewWidget::isAutoRefreshEnabled() const
{
    return m_autoRefreshTimer->isActive();
}

void CameraMapOverviewWidget::onCameraSelected(const QString& cameraIdString)
{
    nx::Uuid cameraId = nx::Uuid::fromString(cameraIdString);
    if (!cameraId.isNull())
    {
        emit cameraSelected(cameraId);
    }
}

void CameraMapOverviewWidget::onCameraDoubleClicked(const QString& cameraIdString)
{
    nx::Uuid cameraId = nx::Uuid::fromString(cameraIdString);
    if (!cameraId.isNull())
    {
        emit cameraDoubleClicked(cameraId);
    }
}

void CameraMapOverviewWidget::onRefreshRequested()
{
    refreshCameras();
}

void CameraMapOverviewWidget::onAutoRefreshTimer()
{
    refreshCameras();
}

void CameraMapOverviewWidget::updateCameraOnMap(const CameraLocationInfo& camera)
{
    QJsonObject cameraObj;
    cameraObj["id"] = camera.id.toString();
    cameraObj["name"] = camera.name;
    cameraObj["latitude"] = camera.latitude;
    cameraObj["longitude"] = camera.longitude;
    cameraObj["status"] = camera.status;
    cameraObj["address"] = camera.address;
    
    QJsonDocument doc(cameraObj);
    QString script = QString("updateCamera(%1);").arg(QString::fromUtf8(doc.toJson(QJsonDocument::Compact)));
    m_webView->page()->runJavaScript(script);
}

QString CameraMapOverviewWidget::getCameraStatusColor(const QString& status) const
{
    if (status == "online") return "#28a745";
    if (status == "offline") return "#dc3545";
    if (status == "recording") return "#007bff";
    if (status == "unauthorized") return "#ffc107";
    return "#666666";
}

QString CameraMapOverviewWidget::getCameraStatusIcon(const QString& status) const
{
    if (status == "online") return "●";
    if (status == "offline") return "●";
    if (status == "recording") return "●";
    if (status == "unauthorized") return "●";
    return "●";
}

} // namespace nx::vms::client::desktop
