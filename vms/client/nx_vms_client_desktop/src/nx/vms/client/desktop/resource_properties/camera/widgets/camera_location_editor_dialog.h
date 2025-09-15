// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#pragma once

#include <QtWidgets/QDialog>
#include <QtCore/QScopedPointer>
#include <optional>

namespace nx::vms::client::desktop {

class CameraGeolocationWidget;
class CameraMapViewWidget;

/**
 * Dialog for editing camera location with both coordinate input and interactive map.
 * Provides synchronized editing between coordinate fields and map visualization.
 */
class CameraLocationEditorDialog: public QDialog
{
    Q_OBJECT

public:
    explicit CameraLocationEditorDialog(QWidget* parent = nullptr);
    virtual ~CameraLocationEditorDialog();

    // Location data
    std::optional<double> latitude() const;
    std::optional<double> longitude() const;
    std::optional<double> altitude() const;
    QString geolocationAddress() const;

    void setLatitude(const std::optional<double>& latitude);
    void setLongitude(const std::optional<double>& longitude);
    void setAltitude(const std::optional<double>& altitude);
    void setGeolocationAddress(const QString& address);

    // Camera information
    void setCameraName(const QString& name);
    QString cameraName() const;

    // Dialog state
    void clearLocation();
    bool hasValidLocation() const;

private slots:
    void onCoordinatesChanged();
    void onAddressChanged();
    void onLocationCleared();
    void onMapLocationSelected(double latitude, double longitude);
    void onCameraMarkerMoved(double latitude, double longitude);
    void onOpenMapRequested();
    void onAccept();
    void onReject();

private:
    void setupUi();
    void updateMapFromCoordinates();
    void updateCoordinatesFromMap(double latitude, double longitude);
    void syncWidgets();

private:
    CameraGeolocationWidget* m_geolocationWidget;
    CameraMapViewWidget* m_mapWidget;
    QString m_cameraName;
    bool m_updatingFromMap;
    bool m_updatingFromWidget;
};

} // namespace nx::vms::client::desktop
