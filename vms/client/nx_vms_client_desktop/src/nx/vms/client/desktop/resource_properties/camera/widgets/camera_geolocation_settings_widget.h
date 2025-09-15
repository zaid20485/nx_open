// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#pragma once

#include <QtWidgets/QWidget>
#include <QtCore/QScopedPointer>

namespace nx::vms::client::desktop {

class CameraSettingsDialogStore;
class CameraGeolocationWidget;
class CameraLocationEditorDialog;

/**
 * Widget for managing camera geolocation settings within the camera settings dialog.
 * Integrates with the camera settings store and provides UI for editing location data.
 */
class CameraGeolocationSettingsWidget: public QWidget
{
    Q_OBJECT

public:
    explicit CameraGeolocationSettingsWidget(
        CameraSettingsDialogStore* store,
        QWidget* parent = nullptr);
    virtual ~CameraGeolocationSettingsWidget();

private slots:
    void onCoordinatesChanged();
    void onAddressChanged();
    void onLocationCleared();
    void onOpenMapEditor();
    void onStoreStateChanged();

private:
    void setupUi();
    void loadDataFromStore();
    void saveDataToStore();

private:
    CameraSettingsDialogStore* m_store;
    CameraGeolocationWidget* m_geolocationWidget;
    CameraLocationEditorDialog* m_mapEditorDialog;
};

} // namespace nx::vms::client::desktop
