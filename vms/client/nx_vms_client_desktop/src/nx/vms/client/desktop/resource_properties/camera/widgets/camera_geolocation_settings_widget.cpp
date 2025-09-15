// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#include "camera_geolocation_settings_widget.h"
#include "camera_geolocation_widget.h"
#include "camera_location_editor_dialog.h"

#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QLabel>

#include "../flux/camera_settings_dialog_store.h"
#include "../flux/camera_settings_dialog_state.h"

namespace nx::vms::client::desktop {

CameraGeolocationSettingsWidget::CameraGeolocationSettingsWidget(
    CameraSettingsDialogStore* store,
    QWidget* parent):
    QWidget(parent),
    m_store(store),
    m_geolocationWidget(new CameraGeolocationWidget(this)),
    m_mapEditorDialog(nullptr)
{
    setupUi();
    
    if (m_store)
    {
        connect(m_store, &CameraSettingsDialogStore::stateChanged,
            this, &CameraGeolocationSettingsWidget::onStoreStateChanged);
        loadDataFromStore();
    }
}

CameraGeolocationSettingsWidget::~CameraGeolocationSettingsWidget()
{
}

void CameraGeolocationSettingsWidget::setupUi()
{
    auto mainLayout = new QVBoxLayout(this);

    // Title and description
    auto titleLabel = new QLabel(tr("Camera Location"), this);
    titleLabel->setStyleSheet("QLabel { font-weight: bold; font-size: 14px; }");
    
    auto descriptionLabel = new QLabel(
        tr("Set the geographic location of this camera for mapping and location-based features."), this);
    descriptionLabel->setWordWrap(true);
    descriptionLabel->setStyleSheet("QLabel { color: #666; margin-bottom: 10px; }");

    mainLayout->addWidget(titleLabel);
    mainLayout->addWidget(descriptionLabel);

    // Geolocation widget
    auto locationGroup = new QGroupBox(tr("Location Information"), this);
    auto locationLayout = new QVBoxLayout(locationGroup);
    locationLayout->addWidget(m_geolocationWidget);

    // Map editor button
    auto buttonLayout = new QHBoxLayout();
    auto mapEditorButton = new QPushButton(tr("Open Map Editor"), this);
    mapEditorButton->setToolTip(tr("Open interactive map editor for precise location selection"));
    buttonLayout->addWidget(mapEditorButton);
    buttonLayout->addStretch();

    locationLayout->addLayout(buttonLayout);
    mainLayout->addWidget(locationGroup);
    mainLayout->addStretch();

    // Connect signals
    connect(m_geolocationWidget, &CameraGeolocationWidget::coordinatesChanged,
        this, &CameraGeolocationSettingsWidget::onCoordinatesChanged);
    connect(m_geolocationWidget, &CameraGeolocationWidget::addressChanged,
        this, &CameraGeolocationSettingsWidget::onAddressChanged);
    connect(m_geolocationWidget, &CameraGeolocationWidget::locationCleared,
        this, &CameraGeolocationSettingsWidget::onLocationCleared);
    connect(m_geolocationWidget, &CameraGeolocationWidget::openMapRequested,
        this, &CameraGeolocationSettingsWidget::onOpenMapEditor);
    connect(mapEditorButton, &QPushButton::clicked,
        this, &CameraGeolocationSettingsWidget::onOpenMapEditor);
}

void CameraGeolocationSettingsWidget::onCoordinatesChanged()
{
    saveDataToStore();
}

void CameraGeolocationSettingsWidget::onAddressChanged()
{
    saveDataToStore();
}

void CameraGeolocationSettingsWidget::onLocationCleared()
{
    saveDataToStore();
}

void CameraGeolocationSettingsWidget::onOpenMapEditor()
{
    if (!m_mapEditorDialog)
    {
        m_mapEditorDialog = new CameraLocationEditorDialog(this);
    }

    // Load current data into dialog
    m_mapEditorDialog->setLatitude(m_geolocationWidget->latitude());
    m_mapEditorDialog->setLongitude(m_geolocationWidget->longitude());
    m_mapEditorDialog->setAltitude(m_geolocationWidget->altitude());
    m_mapEditorDialog->setGeolocationAddress(m_geolocationWidget->geolocationAddress());

    // Set camera name if available
    if (m_store)
    {
        const auto& state = m_store->state();
        if (state.isSingleCamera())
        {
            m_mapEditorDialog->setCameraName(state.singleCameraProperties.name());
        }
    }

    if (m_mapEditorDialog->exec() == QDialog::Accepted)
    {
        // Update widget with dialog results
        m_geolocationWidget->setLatitude(m_mapEditorDialog->latitude());
        m_geolocationWidget->setLongitude(m_mapEditorDialog->longitude());
        m_geolocationWidget->setAltitude(m_mapEditorDialog->altitude());
        m_geolocationWidget->setGeolocationAddress(m_mapEditorDialog->geolocationAddress());
        
        // Save to store
        saveDataToStore();
    }
}

void CameraGeolocationSettingsWidget::onStoreStateChanged()
{
    loadDataFromStore();
}

void CameraGeolocationSettingsWidget::loadDataFromStore()
{
    if (!m_store)
        return;

    const auto& state = m_store->state();
    
    // Block signals to prevent recursive updates
    m_geolocationWidget->blockSignals(true);
    
    m_geolocationWidget->setLatitude(state.geolocation.latitude());
    m_geolocationWidget->setLongitude(state.geolocation.longitude());
    m_geolocationWidget->setAltitude(state.geolocation.altitude());
    m_geolocationWidget->setGeolocationAddress(state.geolocation.address());
    
    m_geolocationWidget->blockSignals(false);
}

void CameraGeolocationSettingsWidget::saveDataToStore()
{
    if (!m_store)
        return;

    // Update store with current widget values
    m_store->setGeolocationLatitude(m_geolocationWidget->latitude());
    m_store->setGeolocationLongitude(m_geolocationWidget->longitude());
    m_store->setGeolocationAltitude(m_geolocationWidget->altitude());
    m_store->setGeolocationAddress(m_geolocationWidget->geolocationAddress());
}

} // namespace nx::vms::client::desktop
