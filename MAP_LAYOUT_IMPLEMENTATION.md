# NX Open Map Layout Implementation Guide

## Overview
This document describes the comprehensive map layout feature implementation for NX Open VMS 4.1 that provides geographical visualization of camera locations with interactive controls and real-time status updates.

## 1. Workspace Analysis

### Project Structure
- **VMS Client Desktop**: `vms/client/nx_vms_client_desktop/` - Main desktop client application
- **Resource Management**: `core/resource_management/` - Camera and resource handling
- **UI Components**: `ui/workbench/` - Workbench and layout management
- **Geolocation Widgets**: `resource_properties/camera/widgets/` - Camera location widgets

### Key Components
- `MapLayoutWidget` - Main map layout visualization widget
- `CameraMapViewWidget` - Individual camera location editor
- `CameraMapOverviewWidget` - System-wide camera overview
- `CameraGeolocationWidget` - Coordinate input and GPS detection

## 2. Geolocation Implementation Corrections

### Fixed Issues
1. **Serialization Errors**: Added `NX_REFLECTION_INSTRUMENT` macro for `GeolocationSettings` struct
2. **Qt Include Paths**: Corrected from `QtCore` to `QtPositioning` for geolocation widgets
3. **String Conversions**: Fixed `QString` to `std::string_view` with `.toStdString()` calls
4. **JavaScript Functions**: Added missing `onMapClicked` slot implementation

### Database Schema
- Added geolocation fields to camera table:
  - `latitude` (DOUBLE)
  - `longitude` (DOUBLE)
  - `altitude` (DOUBLE)
  - `address` (VARCHAR)

## 3. Map Layout Features

### Core Functionality
- **Interactive Map Display**: OpenStreetMap with Leaflet.js integration
- **Camera Markers**: Color-coded by status (online/offline/recording)
- **Clustering**: Automatic grouping of nearby cameras
- **Multiple Map Types**: Street, Satellite, Hybrid, Terrain views
- **Real-time Updates**: Auto-refresh camera statuses every 30 seconds

### User Interface
```
┌─────────────────────────────────────────────────┐
│ [Toolbar]                                       │
│ ┌─────┬──────┬──────┬────────┬──────┬────────┐ │
│ │Refresh│Fit All│Map Type│ Zoom │Group│Settings│ │
│ └─────┴──────┴──────┴────────┴──────┴────────┘ │
├─────────────────────────────────────────────────┤
│                                                 │
│              [Interactive Map]                   │
│                                                 │
│    🟢 Camera 1 (Recording)                      │
│    🔴 Camera 2 (Offline)                        │
│    🟢 Camera 3 (Online)                         │
│                                                 │
├─────────────────────────────────────────────────┤
│ Total: 3 | Online: 2 | Recording: 1 | Offline: 1│
└─────────────────────────────────────────────────┘
```

### Camera Status Indicators
- 🟢 **Green**: Camera online
- 🔵 **Blue**: Camera recording (with pulse animation)
- 🔴 **Red**: Camera offline
- 🟠 **Orange**: Camera unauthorized

## 4. Integration with Main Application

### Menu Integration
- **Shortcut**: `Ctrl+M` to open map layout
- **Menu Path**: View → Camera Map
- **Toolbar**: Quick access button in main toolbar

### Code Usage Example
```cpp
#include <nx/vms/client/desktop/resource_views/map_layout/map_layout_widget.h>
#include <nx/vms/client/desktop/ui/actions/map_layout_actions.h>

// Open map layout programmatically
void openCameraMap(WindowContext* context) {
    ui::actions::openMapLayout(context);
}

// Create map widget manually
auto mapWidget = new MapLayoutWidget(windowContext);
mapWidget->loadCamerasFromSystem();
mapWidget->setAutoRefresh(true, 30000);
```

## 5. API Reference

### MapLayoutWidget Public Methods

#### Camera Management
```cpp
void addCamera(const CameraInfo& camera);
void updateCamera(const CameraInfo& camera);
void removeCamera(const nx::Uuid& cameraId);
void clearAllCameras();
void loadCamerasFromSystem();
```

#### Map Controls
```cpp
void centerOnCamera(const nx::Uuid& cameraId);
void fitAllCameras();
void setMapType(const QString& mapType);
void setZoomLevel(int zoom);
void groupCamerasByClusters(bool enabled);
```

#### Filtering
```cpp
void setStatusFilter(const QStringList& statuses);
void setNameFilter(const QString& pattern);
void setLocationBounds(double minLat, double maxLat, double minLon, double maxLon);
```

#### Export/Import
```cpp
void exportMapView(const QString& filePath);
void importCameraLocations(const QString& filePath);
```

## 6. Configuration

### Map Settings
```json
{
  "map_layout": {
    "default_map_type": "street",
    "default_zoom": 10,
    "clustering_enabled": true,
    "auto_refresh": true,
    "refresh_interval_ms": 30000,
    "marker_settings": {
      "size": 30,
      "show_labels": true,
      "animation_enabled": true
    }
  }
}
```

## 7. Testing

### Test Scenarios
1. **Add Camera Location**: Set camera coordinates via settings dialog
2. **Map Display**: Verify camera appears on map at correct location
3. **Status Updates**: Check real-time status changes reflected on map
4. **Clustering**: Test grouping with 10+ cameras in close proximity
5. **Export/Import**: Verify location data persistence

### Sample Test Data
```cpp
// Add test camera with location
CameraInfo testCamera;
testCamera.id = nx::Uuid::createUuid();
testCamera.name = "Test Camera 01";
testCamera.latitude = 40.7128;
testCamera.longitude = -74.0060;
testCamera.status = "online";
mapWidget->addCamera(testCamera);
```

## 8. Performance Considerations

### Optimization Strategies
- **Viewport Loading**: Only load cameras visible in current map bounds
- **Marker Clustering**: Reduce DOM elements with automatic grouping
- **Debounced Updates**: Batch status updates to minimize redraws
- **Lazy Loading**: Load camera details on-demand when selected

### Recommended Limits
- Maximum cameras without clustering: 100
- Maximum cameras with clustering: 1000+
- Refresh interval minimum: 10 seconds
- Map zoom levels: 1-20

## 9. Troubleshooting

### Common Issues

#### Cameras Not Appearing on Map
- Verify camera has valid latitude/longitude in database
- Check resource pool connection is active
- Ensure map widget is properly initialized

#### JavaScript Errors
- Check WebChannel connection status
- Verify Leaflet.js library loaded correctly
- Inspect browser console in debug mode

#### Performance Issues
- Enable clustering for >100 cameras
- Increase refresh interval
- Reduce map detail level (use street vs satellite)

## 10. Future Enhancements

### Planned Features
- [ ] Heatmap visualization for camera density
- [ ] Historical playback of camera movements
- [ ] Geofencing and alert zones
- [ ] Integration with GPS tracking devices
- [ ] Custom map tile servers support
- [ ] 3D building visualization
- [ ] Route planning between cameras
- [ ] Weather overlay integration

## Conclusion

The map layout implementation provides a powerful geographical visualization tool for NX Open VMS, enabling operators to quickly assess system status and navigate between camera locations. The modular design allows for easy extension and customization to meet specific deployment requirements.

For additional support or feature requests, please refer to the NX Open development documentation.
