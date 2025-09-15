// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#include "camera_location_editor_dialog.h"
#include "camera_geolocation_widget.h"
#include "camera_map_view_widget.h"

#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QGroupBox>

namespace nx::vms::client::desktop {

CameraLocationEditorDialog::CameraLocationEditorDialog(QWidget* parent):
    QDialog(parent),
    m_geolocationWidget(new CameraGeolocationWidget(this)),
    m_mapWidget(new CameraMapViewWidget(this)),
    m_updatingFromMap(false),
    m_updatingFromWidget(false)
{
    setupUi();
}

CameraLocationEditorDialog::~CameraLocationEditorDialog()
{
}

void CameraLocationEditorDialog::setupUi()
{
    setWindowTitle(tr("Edit Camera Location"));
    setModal(true);
    resize(800, 600);

    auto mainLayout = new QVBoxLayout(this);

    // Title
    auto titleLabel = new QLabel(tr("Set the geographic location for this camera"), this);
    titleLabel->setStyleSheet("QLabel { font-weight: bold; margin-bottom: 10px; }");
    mainLayout->addWidget(titleLabel);

    // Main content splitter
    auto splitter = new QSplitter(Qt::Horizontal, this);
    
    // Left side - coordinate input
    auto leftWidget = new QWidget();
    auto leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->addWidget(m_geolocationWidget);
    leftLayout->addStretch();
    
    // Right side - map view
    auto mapGroup = new QGroupBox(tr("Interactive Map"), this);
    auto mapLayout = new QVBoxLayout(mapGroup);
    mapLayout->addWidget(m_mapWidget);
    
    splitter->addWidget(leftWidget);
    splitter->addWidget(mapGroup);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 2);
    
    mainLayout->addWidget(splitter);

    // Dialog buttons
    auto buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    mainLayout->addWidget(buttonBox);

    // Connect signals
    connect(m_geolocationWidget, &CameraGeolocationWidget::coordinatesChanged,
        this, &CameraLocationEditorDialog::onCoordinatesChanged);
    connect(m_geolocationWidget, &CameraGeolocationWidget::addressChanged,
        this, &CameraLocationEditorDialog::onAddressChanged);
    connect(m_geolocationWidget, &CameraGeolocationWidget::locationCleared,
        this, &CameraLocationEditorDialog::onLocationCleared);
    connect(m_geolocationWidget, &CameraGeolocationWidget::openMapRequested,
        this, &CameraLocationEditorDialog::onOpenMapRequested);

    connect(m_mapWidget, &CameraMapViewWidget::locationSelected,
        this, &CameraLocationEditorDialog::onMapLocationSelected);
    connect(m_mapWidget, &CameraMapViewWidget::cameraMarkerMoved,
        this, &CameraLocationEditorDialog::onCameraMarkerMoved);

    connect(buttonBox, &QDialogButtonBox::accepted,
        this, &CameraLocationEditorDialog::onAccept);
    connect(buttonBox, &QDialogButtonBox::rejected,
        this, &CameraLocationEditorDialog::onReject);

    // Enable location selection mode on map by default
    m_mapWidget->setLocationSelectionMode(true);
}

std::optional<double> CameraLocationEditorDialog::latitude() const
{
    return m_geolocationWidget->latitude();
}

std::optional<double> CameraLocationEditorDialog::longitude() const
{
    return m_geolocationWidget->longitude();
}

std::optional<double> CameraLocationEditorDialog::altitude() const
{
    return m_geolocationWidget->altitude();
}

QString CameraLocationEditorDialog::geolocationAddress() const
{
    return m_geolocationWidget->geolocationAddress();
}

void CameraLocationEditorDialog::setLatitude(const std::optional<double>& latitude)
{
    m_geolocationWidget->setLatitude(latitude);
    updateMapFromCoordinates();
}

void CameraLocationEditorDialog::setLongitude(const std::optional<double>& longitude)
{
    m_geolocationWidget->setLongitude(longitude);
    updateMapFromCoordinates();
}

void CameraLocationEditorDialog::setAltitude(const std::optional<double>& altitude)
{
    m_geolocationWidget->setAltitude(altitude);
}

void CameraLocationEditorDialog::setGeolocationAddress(const QString& address)
{
    m_geolocationWidget->setGeolocationAddress(address);
}

void CameraLocationEditorDialog::setCameraName(const QString& name)
{
    m_cameraName = name;
    updateMapFromCoordinates();
}

QString CameraLocationEditorDialog::cameraName() const
{
    return m_cameraName;
}

void CameraLocationEditorDialog::clearLocation()
{
    m_geolocationWidget->clearLocation();
    m_mapWidget->clearCameraLocation();
}

bool CameraLocationEditorDialog::hasValidLocation() const
{
    return m_geolocationWidget->hasValidCoordinates();
}

void CameraLocationEditorDialog::onCoordinatesChanged()
{
    if (!m_updatingFromMap)
    {
        m_updatingFromWidget = true;
        updateMapFromCoordinates();
        m_updatingFromWidget = false;
    }
}

void CameraLocationEditorDialog::onAddressChanged()
{
    // Address changes don't affect map view
}

void CameraLocationEditorDialog::onLocationCleared()
{
    m_mapWidget->clearCameraLocation();
}

void CameraLocationEditorDialog::onMapLocationSelected(double latitude, double longitude)
{
    if (!m_updatingFromWidget)
    {
        m_updatingFromMap = true;
        updateCoordinatesFromMap(latitude, longitude);
        m_updatingFromMap = false;
    }
}

void CameraLocationEditorDialog::onCameraMarkerMoved(double latitude, double longitude)
{
    if (!m_updatingFromWidget)
    {
        m_updatingFromMap = true;
        updateCoordinatesFromMap(latitude, longitude);
        m_updatingFromMap = false;
    }
}

void CameraLocationEditorDialog::onOpenMapRequested()
{
    // Map is already visible, just focus on it or center on current location
    auto lat = latitude();
    auto lon = longitude();
    if (lat.has_value() && lon.has_value())
    {
        m_mapWidget->centerOnLocation(lat.value(), lon.value());
    }
}

void CameraLocationEditorDialog::onAccept()
{
    if (hasValidLocation())
    {
        accept();
    }
    else
    {
        // Could show validation message, but for now just accept anyway
        accept();
    }
}

void CameraLocationEditorDialog::onReject()
{
    reject();
}

void CameraLocationEditorDialog::updateMapFromCoordinates()
{
    auto lat = latitude();
    auto lon = longitude();
    
    if (lat.has_value() && lon.has_value())
    {
        m_mapWidget->setCameraLocation(lat, lon, m_cameraName);
    }
    else
    {
        m_mapWidget->clearCameraLocation();
    }
}

void CameraLocationEditorDialog::updateCoordinatesFromMap(double latitude, double longitude)
{
    m_geolocationWidget->setLatitude(latitude);
    m_geolocationWidget->setLongitude(longitude);
    
    // Update map marker
    m_mapWidget->setCameraLocation(latitude, longitude, m_cameraName);
}

void CameraLocationEditorDialog::syncWidgets()
{
    updateMapFromCoordinates();
}

} // namespace nx::vms::client::desktop
