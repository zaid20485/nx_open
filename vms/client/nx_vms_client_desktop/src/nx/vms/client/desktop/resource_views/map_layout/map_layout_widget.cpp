// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#include "map_layout_widget.h"

#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QLabel>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QSlider>
#include <QtWidgets/QFileDialog>
#include <QtGui/QAction>
#include <QtWebEngineWidgets/QWebEngineView>
#include <QtWebChannel/QWebChannel>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>

#include <core/resource/camera_resource.h>
#include <core/resource/layout_resource.h>
#include <core/resource_management/resource_pool.h>
#include <nx/vms/client/desktop/system_context.h>
#include <nx/vms/client/desktop/window_context.h>
#include <nx/vms/client/desktop/workbench/workbench.h>
#include <nx/vms/client/core/skin/skin.h>
#include <ui/workbench/workbench_context.h>

namespace nx::vms::client::desktop {

struct MapLayoutWidget::Private
{
    MapLayoutWidget* q;
    QWebEngineView* webView = nullptr;
    QWebChannel* webChannel = nullptr;
    QToolBar* toolbar = nullptr;
    QComboBox* mapTypeCombo = nullptr;
    QSlider* zoomSlider = nullptr;
    QLabel* statusLabel = nullptr;
    QTimer* autoRefreshTimer = nullptr;
    
    QnLayoutResourcePtr layout;
    QList<CameraInfo> cameras;
    QStringList statusFilter;
    QString nameFilter;
    bool mapReady = false;
    bool clusteringEnabled = true;
    QString currentMapType = "street";
    int currentZoom = 10;
    
    // Actions
    QAction* refreshAction = nullptr;
    QAction* fitAllAction = nullptr;
    QAction* fullscreenAction = nullptr;
    QAction* exportAction = nullptr;
    QAction* importAction = nullptr;
    QAction* settingsAction = nullptr;
    QAction* clusterAction = nullptr;
    
    Private(MapLayoutWidget* parent): q(parent) {}
};

MapLayoutWidget::MapLayoutWidget(WindowContext* windowContext, QWidget* parent):
    QWidget(parent),
    WindowContextAware(windowContext),
    d(new Private(this))
{
    setupUi();
    setupToolbar();
    connectToResourcePool();
    
    d->autoRefreshTimer = new QTimer(this);
    connect(d->autoRefreshTimer, &QTimer::timeout,
        this, &MapLayoutWidget::onAutoRefreshTimer);
}

MapLayoutWidget::~MapLayoutWidget()
{
}

void MapLayoutWidget::setupUi()
{
    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    
    // Create toolbar
    d->toolbar = new QToolBar(this);
    d->toolbar->setMovable(false);
    mainLayout->addWidget(d->toolbar);
    
    // Create web view for map
    d->webView = new QWebEngineView(this);
    d->webChannel = new QWebChannel(this);
    d->webView->page()->setWebChannel(d->webChannel);
    d->webChannel->registerObject("mapController", this);
    
    mainLayout->addWidget(d->webView);
    
    // Status bar
    d->statusLabel = new QLabel(this);
    d->statusLabel->setStyleSheet("QLabel { padding: 5px; background: #f0f0f0; }");
    mainLayout->addWidget(d->statusLabel);
    
    // Connect web view signals
    connect(d->webView, &QWebEngineView::loadFinished,
        this, &MapLayoutWidget::onMapReady);
    
    loadMapHtml();
}

void MapLayoutWidget::setupToolbar()
{
    // Refresh action
    d->refreshAction = d->toolbar->addAction(
        qnSkin->icon("tree/reload.svg"), "Refresh");
    connect(d->refreshAction, &QAction::triggered,
        this, &MapLayoutWidget::refreshCameraStatuses);
    
    // Fit all cameras action
    d->fitAllAction = d->toolbar->addAction(
        qnSkin->icon("tree/expand.svg"), "Fit All Cameras");
    connect(d->fitAllAction, &QAction::triggered,
        this, &MapLayoutWidget::fitAllCameras);
    
    d->toolbar->addSeparator();
    
    // Map type selector
    d->toolbar->addWidget(new QLabel("Map Type:", d->toolbar));
    d->mapTypeCombo = new QComboBox(d->toolbar);
    d->mapTypeCombo->addItems({"Street", "Satellite", "Hybrid", "Terrain"});
    connect(d->mapTypeCombo, &QComboBox::currentTextChanged,
        [this](const QString& text) {
            setMapType(text.toLower());
        });
    d->toolbar->addWidget(d->mapTypeCombo);
    
    d->toolbar->addSeparator();
    
    // Zoom control
    d->toolbar->addWidget(new QLabel("Zoom:", d->toolbar));
    d->zoomSlider = new QSlider(Qt::Horizontal, d->toolbar);
    d->zoomSlider->setRange(1, 20);
    d->zoomSlider->setValue(10);
    d->zoomSlider->setFixedWidth(150);
    connect(d->zoomSlider, &QSlider::valueChanged,
        this, &MapLayoutWidget::setZoomLevel);
    d->toolbar->addWidget(d->zoomSlider);
    
    d->toolbar->addSeparator();
    
    // Clustering toggle
    d->clusterAction = d->toolbar->addAction(
        qnSkin->icon("tree/group.svg"), "Group Cameras");
    d->clusterAction->setCheckable(true);
    d->clusterAction->setChecked(true);
    connect(d->clusterAction, &QAction::toggled,
        this, &MapLayoutWidget::groupCamerasByClusters);
    
    d->toolbar->addSeparator();
    
    // Export/Import actions
    d->exportAction = d->toolbar->addAction(
        qnSkin->icon("tree/export.svg"), "Export View");
    connect(d->exportAction, &QAction::triggered,
        this, &MapLayoutWidget::onExportView);
    
    d->importAction = d->toolbar->addAction(
        qnSkin->icon("tree/import.svg"), "Import Locations");
    connect(d->importAction, &QAction::triggered,
        this, &MapLayoutWidget::onImportLocations);
    
    // Spacer
    auto spacer = new QWidget(d->toolbar);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    d->toolbar->addWidget(spacer);
    
    // Fullscreen action
    d->fullscreenAction = d->toolbar->addAction(
        qnSkin->icon("tree/fullscreen.svg"), "Fullscreen");
    connect(d->fullscreenAction, &QAction::triggered,
        this, &MapLayoutWidget::onToggleFullscreen);
    
    // Settings action
    d->settingsAction = d->toolbar->addAction(
        qnSkin->icon("tree/settings.svg"), "Settings");
    connect(d->settingsAction, &QAction::triggered,
        this, &MapLayoutWidget::onShowSettings);
}

void MapLayoutWidget::loadMapHtml()
{
    const QString mapHtml = R"(
<!DOCTYPE html>
<html>
<head>
    <title>Camera Map Layout</title>
    <meta charset="utf-8" />
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <link rel="stylesheet" href="https://unpkg.com/leaflet@1.9.4/dist/leaflet.css" />
    <link rel="stylesheet" href="https://unpkg.com/leaflet.markercluster@1.5.3/dist/MarkerCluster.css" />
    <link rel="stylesheet" href="https://unpkg.com/leaflet.markercluster@1.5.3/dist/MarkerCluster.Default.css" />
    <style>
        body { margin: 0; padding: 0; }
        #map { height: 100vh; width: 100%; }
        .camera-popup { min-width: 200px; }
        .camera-popup h4 { margin: 5px 0; }
        .camera-popup .status { font-weight: bold; }
        .camera-popup .status.online { color: #4CAF50; }
        .camera-popup .status.offline { color: #f44336; }
        .camera-popup .status.recording { color: #2196F3; }
        .camera-popup button { margin-top: 10px; padding: 5px 10px; }
    </style>
</head>
<body>
    <div id="map"></div>
    
    <script src="https://unpkg.com/leaflet@1.9.4/dist/leaflet.js"></script>
    <script src="https://unpkg.com/leaflet.markercluster@1.5.3/dist/leaflet.markercluster.js"></script>
    <script src="qrc:///qtwebchannel/qwebchannel.js"></script>
    <script>
        let map;
        let cameras = {};
        let markers = {};
        let markerClusterGroup;
        let mapController;
        let clusteringEnabled = true;
        
        // Custom camera icons
        const cameraIcons = {
            online: L.divIcon({
                html: '<div style="background-color: #4CAF50; width: 30px; height: 30px; border-radius: 50%; border: 3px solid white; box-shadow: 0 2px 5px rgba(0,0,0,0.3);"></div>',
                className: 'camera-marker-online',
                iconSize: [30, 30]
            }),
            offline: L.divIcon({
                html: '<div style="background-color: #f44336; width: 30px; height: 30px; border-radius: 50%; border: 3px solid white; box-shadow: 0 2px 5px rgba(0,0,0,0.3);"></div>',
                className: 'camera-marker-offline',
                iconSize: [30, 30]
            }),
            recording: L.divIcon({
                html: '<div style="background-color: #2196F3; width: 30px; height: 30px; border-radius: 50%; border: 3px solid white; box-shadow: 0 2px 5px rgba(0,0,0,0.3); animation: pulse 2s infinite;"></div>',
                className: 'camera-marker-recording',
                iconSize: [30, 30]
            }),
            unauthorized: L.divIcon({
                html: '<div style="background-color: #FF9800; width: 30px; height: 30px; border-radius: 50%; border: 3px solid white; box-shadow: 0 2px 5px rgba(0,0,0,0.3);"></div>',
                className: 'camera-marker-unauthorized',
                iconSize: [30, 30]
            })
        };
        
        // Add pulsing animation for recording cameras
        const style = document.createElement('style');
        style.innerHTML = `
            @keyframes pulse {
                0% { transform: scale(1); opacity: 1; }
                50% { transform: scale(1.1); opacity: 0.8; }
                100% { transform: scale(1); opacity: 1; }
            }
        `;
        document.head.appendChild(style);
        
        // Initialize map
        function initMap() {
            map = L.map('map').setView([40.7128, -74.0060], 10);
            
            // Add tile layer
            L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png', {
                attribution: '© OpenStreetMap contributors'
            }).addTo(map);
            
            // Initialize marker cluster group
            markerClusterGroup = L.markerClusterGroup({
                maxClusterRadius: 80,
                spiderfyOnMaxZoom: true,
                showCoverageOnHover: true,
                zoomToBoundsOnClick: true
            });
            
            if (clusteringEnabled) {
                map.addLayer(markerClusterGroup);
            }
            
            // Map events
            map.on('zoomend', function() {
                if (mapController) {
                    mapController.onMapZoomChanged(map.getZoom());
                }
            });
            
            map.on('moveend', function() {
                const bounds = map.getBounds();
                if (mapController) {
                    mapController.onMapBoundsChanged(
                        bounds.getSouth(), bounds.getNorth(),
                        bounds.getWest(), bounds.getEast()
                    );
                }
            });
        }
        
        // Add or update camera marker
        function addCamera(id, name, lat, lng, status, address) {
            const camera = {
                id: id,
                name: name,
                lat: lat,
                lng: lng,
                status: status,
                address: address
            };
            
            cameras[id] = camera;
            
            // Remove existing marker if it exists
            if (markers[id]) {
                if (clusteringEnabled) {
                    markerClusterGroup.removeLayer(markers[id]);
                } else {
                    map.removeLayer(markers[id]);
                }
                delete markers[id];
            }
            
            // Create new marker
            const icon = cameraIcons[status] || cameraIcons.offline;
            const marker = L.marker([lat, lng], { icon: icon });
            
            // Create popup content
            const popupContent = `
                <div class="camera-popup">
                    <h4>${name}</h4>
                    <p>Status: <span class="status ${status}">${status.toUpperCase()}</span></p>
                    <p>Location: ${lat.toFixed(6)}, ${lng.toFixed(6)}</p>
                    ${address ? '<p>Address: ' + address + '</p>' : ''}
                    <button onclick="viewCamera('${id}')">View Camera</button>
                </div>
            `;
            
            marker.bindPopup(popupContent);
            
            // Handle marker events
            marker.on('click', function() {
                if (mapController) {
                    mapController.onCameraSelectedInMap(id);
                }
            });
            
            marker.on('dblclick', function() {
                if (mapController) {
                    mapController.onCameraDoubleClickedInMap(id);
                }
            });
            
            markers[id] = marker;
            
            // Add to map
            if (clusteringEnabled) {
                markerClusterGroup.addLayer(marker);
            } else {
                marker.addTo(map);
            }
        }
        
        // Remove camera marker
        function removeCamera(id) {
            if (markers[id]) {
                if (clusteringEnabled) {
                    markerClusterGroup.removeLayer(markers[id]);
                } else {
                    map.removeLayer(markers[id]);
                }
                delete markers[id];
                delete cameras[id];
            }
        }
        
        // Clear all cameras
        function clearAllCameras() {
            if (clusteringEnabled) {
                markerClusterGroup.clearLayers();
            } else {
                for (let id in markers) {
                    map.removeLayer(markers[id]);
                }
            }
            markers = {};
            cameras = {};
        }
        
        // Center on specific camera
        function centerOnCamera(id) {
            const camera = cameras[id];
            if (camera) {
                map.setView([camera.lat, camera.lng], 16);
                if (markers[id]) {
                    markers[id].openPopup();
                }
            }
        }
        
        // Fit all cameras in view
        function fitAllCameras() {
            if (Object.keys(cameras).length === 0) return;
            
            const bounds = [];
            for (let id in cameras) {
                bounds.push([cameras[id].lat, cameras[id].lng]);
            }
            
            map.fitBounds(bounds, { padding: [50, 50] });
        }
        
        // Set map type
        function setMapType(type) {
            // Clear existing tile layers
            map.eachLayer(function(layer) {
                if (layer instanceof L.TileLayer) {
                    map.removeLayer(layer);
                }
            });
            
            let tileUrl;
            switch(type) {
                case 'satellite':
                    tileUrl = 'https://server.arcgisonline.com/ArcGIS/rest/services/World_Imagery/MapServer/tile/{z}/{y}/{x}';
                    break;
                case 'hybrid':
                    tileUrl = 'https://mt1.google.com/vt/lyrs=y&x={x}&y={y}&z={z}';
                    break;
                case 'terrain':
                    tileUrl = 'https://server.arcgisonline.com/ArcGIS/rest/services/World_Topo_Map/MapServer/tile/{z}/{y}/{x}';
                    break;
                default: // street
                    tileUrl = 'https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png';
            }
            
            L.tileLayer(tileUrl, {
                attribution: '© Map contributors'
            }).addTo(map);
        }
        
        // Set zoom level
        function setZoomLevel(zoom) {
            map.setZoom(zoom);
        }
        
        // Toggle clustering
        function setClusteringEnabled(enabled) {
            if (enabled === clusteringEnabled) return;
            
            clusteringEnabled = enabled;
            
            if (enabled) {
                // Enable clustering
                map.addLayer(markerClusterGroup);
                for (let id in markers) {
                    map.removeLayer(markers[id]);
                    markerClusterGroup.addLayer(markers[id]);
                }
            } else {
                // Disable clustering
                map.removeLayer(markerClusterGroup);
                for (let id in markers) {
                    markers[id].addTo(map);
                }
            }
        }
        
        // View camera function for popup button
        function viewCamera(id) {
            if (mapController) {
                mapController.onCameraDoubleClickedInMap(id);
            }
        }
        
        // Initialize when page loads
        document.addEventListener('DOMContentLoaded', function() {
            initMap();
            
            // Setup Qt WebChannel
            new QWebChannel(qt.webChannelTransport, function(channel) {
                mapController = channel.objects.mapController;
                console.log('WebChannel connected to map controller');
            });
        });
    </script>
</body>
</html>
)";
    
    d->webView->setHtml(mapHtml);
}

void MapLayoutWidget::connectToResourcePool()
{
    auto resourcePool = systemContext()->resourcePool();
    
    connect(resourcePool, &QnResourcePool::resourceAdded,
        this, &MapLayoutWidget::onCameraResourceAdded);
    connect(resourcePool, &QnResourcePool::resourceRemoved,
        this, &MapLayoutWidget::onCameraResourceRemoved);
}

void MapLayoutWidget::loadCamerasFromSystem()
{
    clearAllCameras();
    
    auto resourcePool = systemContext()->resourcePool();
    auto cameras = resourcePool->getAllCameras();
    
    for (const auto& camera : cameras)
    {
        // For now, add all cameras with dummy coordinates
        // TODO: Integrate with actual geolocation data when available
        CameraInfo info;
        info.id = camera->getId();
        info.name = camera->getName();
        info.resource = camera;
        info.latitude = 40.7128 + (qrand() % 100) * 0.001; // Dummy coordinates around NYC
        info.longitude = -74.0060 + (qrand() % 100) * 0.001;
        info.status = camera->isOnline() ? "online" : "offline";
        info.address = ""; // No address for now
        addCamera(info);
    }
    
    fitAllCameras();
}

void MapLayoutWidget::addCamera(const CameraInfo& camera)
{
    d->cameras.append(camera);
    
    if (d->mapReady)
    {
        QString script = QString("addCamera('%1', '%2', %3, %4, '%5', '%6');")
            .arg(camera.id.toString())
            .arg(camera.name)
            .arg(camera.latitude)
            .arg(camera.longitude)
            .arg(camera.status)
            .arg(camera.address);
        d->webView->page()->runJavaScript(script);
    }
}

void MapLayoutWidget::clearAllCameras()
{
    d->cameras.clear();
    
    if (d->mapReady)
    {
        d->webView->page()->runJavaScript("clearAllCameras();");
    }
}

void MapLayoutWidget::fitAllCameras()
{
    if (d->mapReady)
    {
        d->webView->page()->runJavaScript("fitAllCameras();");
    }
}

void MapLayoutWidget::setAutoRefresh(bool enabled, int intervalMs)
{
    if (enabled)
    {
        d->autoRefreshTimer->start(intervalMs);
    }
    else
    {
        d->autoRefreshTimer->stop();
    }
}

void MapLayoutWidget::onMapReady()
{
    d->mapReady = true;
    loadCamerasFromSystem();
}

void MapLayoutWidget::onCameraResourceAdded(const QnResourcePtr& resource)
{
    auto camera = resource.dynamicCast<QnVirtualCameraResource>();
    if (!camera)
        return;
    
    // Add camera with dummy coordinates for now
    CameraInfo info;
    info.id = camera->getId();
    info.name = camera->getName();
    info.resource = camera;
    info.latitude = 40.7128 + (qrand() % 100) * 0.001;
    info.longitude = -74.0060 + (qrand() % 100) * 0.001;
    info.status = camera->isOnline() ? "online" : "offline";
    info.address = "";
    addCamera(info);
}

void MapLayoutWidget::onCameraResourceRemoved(const QnResourcePtr& resource)
{
    auto camera = resource.dynamicCast<QnVirtualCameraResource>();
    if (!camera)
        return;
    
    // Remove camera from list
    d->cameras.removeIf([&camera](const CameraInfo& info) {
        return info.id == camera->getId();
    });
    
    if (d->mapReady)
    {
        QString script = QString("removeCamera('%1');").arg(camera->getId().toString());
        d->webView->page()->runJavaScript(script);
    }
}

} // namespace nx::vms::client::desktop
