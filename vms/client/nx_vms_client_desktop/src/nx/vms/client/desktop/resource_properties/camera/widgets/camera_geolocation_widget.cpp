// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#include "camera_geolocation_widget.h"

#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QLabel>
#include <QtWidgets/QGroupBox>
#include <QtCore/QGeoPositionInfoSource>
#include <QtCore/QGeoPositionInfo>

#include <nx/vms/client/desktop/style/helper.h>

namespace nx::vms::client::desktop {

CameraGeolocationWidget::CameraGeolocationWidget(QWidget* parent):
    QWidget(parent),
    m_latitudeSpinBox(new QDoubleSpinBox(this)),
    m_longitudeSpinBox(new QDoubleSpinBox(this)),
    m_altitudeSpinBox(new QDoubleSpinBox(this)),
    m_addressEdit(new QLineEdit(this)),
    m_detectGpsButton(new QPushButton(tr("Detect GPS Location"), this)),
    m_clearLocationButton(new QPushButton(tr("Clear Location"), this)),
    m_openMapButton(new QPushButton(tr("Open Map"), this)),
    m_validationLabel(new QLabel(this))
{
    setupUi();
}

CameraGeolocationWidget::~CameraGeolocationWidget()
{
}

void CameraGeolocationWidget::setupUi()
{
    auto mainLayout = new QVBoxLayout(this);

    // Coordinates group
    auto coordinatesGroup = new QGroupBox(tr("Coordinates"), this);
    auto coordinatesLayout = new QFormLayout(coordinatesGroup);

    // Configure coordinate spin boxes
    m_latitudeSpinBox->setRange(-90.0, 90.0);
    m_latitudeSpinBox->setDecimals(6);
    m_latitudeSpinBox->setSuffix("°");
    m_latitudeSpinBox->setSpecialValueText(tr("Not set"));
    m_latitudeSpinBox->setValue(m_latitudeSpinBox->minimum());

    m_longitudeSpinBox->setRange(-180.0, 180.0);
    m_longitudeSpinBox->setDecimals(6);
    m_longitudeSpinBox->setSuffix("°");
    m_longitudeSpinBox->setSpecialValueText(tr("Not set"));
    m_longitudeSpinBox->setValue(m_longitudeSpinBox->minimum());

    m_altitudeSpinBox->setRange(-1000.0, 10000.0);
    m_altitudeSpinBox->setDecimals(1);
    m_altitudeSpinBox->setSuffix(" m");
    m_altitudeSpinBox->setSpecialValueText(tr("Not set"));
    m_altitudeSpinBox->setValue(m_altitudeSpinBox->minimum());

    coordinatesLayout->addRow(tr("Latitude:"), m_latitudeSpinBox);
    coordinatesLayout->addRow(tr("Longitude:"), m_longitudeSpinBox);
    coordinatesLayout->addRow(tr("Altitude:"), m_altitudeSpinBox);

    // Address group
    auto addressGroup = new QGroupBox(tr("Address"), this);
    auto addressLayout = new QVBoxLayout(addressGroup);
    
    m_addressEdit->setPlaceholderText(tr("Enter human-readable address"));
    addressLayout->addWidget(m_addressEdit);

    // Buttons layout
    auto buttonsLayout = new QHBoxLayout();
    buttonsLayout->addWidget(m_detectGpsButton);
    buttonsLayout->addWidget(m_clearLocationButton);
    buttonsLayout->addWidget(m_openMapButton);
    buttonsLayout->addStretch();

    // Validation label
    m_validationLabel->setStyleSheet("QLabel { color: red; }");
    m_validationLabel->hide();

    // Main layout
    mainLayout->addWidget(coordinatesGroup);
    mainLayout->addWidget(addressGroup);
    mainLayout->addLayout(buttonsLayout);
    mainLayout->addWidget(m_validationLabel);
    mainLayout->addStretch();

    // Connect signals
    connect(m_latitudeSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
        this, &CameraGeolocationWidget::onCoordinateChanged);
    connect(m_longitudeSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
        this, &CameraGeolocationWidget::onCoordinateChanged);
    connect(m_altitudeSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
        this, &CameraGeolocationWidget::onCoordinateChanged);
    connect(m_addressEdit, &QLineEdit::textChanged,
        this, &CameraGeolocationWidget::onAddressChanged);
    connect(m_detectGpsButton, &QPushButton::clicked,
        this, &CameraGeolocationWidget::onDetectGpsLocation);
    connect(m_clearLocationButton, &QPushButton::clicked,
        this, &CameraGeolocationWidget::onClearLocation);
    connect(m_openMapButton, &QPushButton::clicked,
        this, &CameraGeolocationWidget::onOpenMap);

    updateValidationState();
}

std::optional<double> CameraGeolocationWidget::latitude() const
{
    if (m_latitudeSpinBox->value() == m_latitudeSpinBox->minimum())
        return std::nullopt;
    return m_latitudeSpinBox->value();
}

std::optional<double> CameraGeolocationWidget::longitude() const
{
    if (m_longitudeSpinBox->value() == m_longitudeSpinBox->minimum())
        return std::nullopt;
    return m_longitudeSpinBox->value();
}

std::optional<double> CameraGeolocationWidget::altitude() const
{
    if (m_altitudeSpinBox->value() == m_altitudeSpinBox->minimum())
        return std::nullopt;
    return m_altitudeSpinBox->value();
}

QString CameraGeolocationWidget::geolocationAddress() const
{
    return m_addressEdit->text();
}

void CameraGeolocationWidget::setLatitude(const std::optional<double>& latitude)
{
    if (latitude.has_value())
        m_latitudeSpinBox->setValue(latitude.value());
    else
        m_latitudeSpinBox->setValue(m_latitudeSpinBox->minimum());
}

void CameraGeolocationWidget::setLongitude(const std::optional<double>& longitude)
{
    if (longitude.has_value())
        m_longitudeSpinBox->setValue(longitude.value());
    else
        m_longitudeSpinBox->setValue(m_longitudeSpinBox->minimum());
}

void CameraGeolocationWidget::setAltitude(const std::optional<double>& altitude)
{
    if (altitude.has_value())
        m_altitudeSpinBox->setValue(altitude.value());
    else
        m_altitudeSpinBox->setValue(m_altitudeSpinBox->minimum());
}

void CameraGeolocationWidget::setGeolocationAddress(const QString& address)
{
    m_addressEdit->setText(address);
}

void CameraGeolocationWidget::clearLocation()
{
    setLatitude(std::nullopt);
    setLongitude(std::nullopt);
    setAltitude(std::nullopt);
    setGeolocationAddress(QString());
    updateValidationState();
}

bool CameraGeolocationWidget::hasValidCoordinates() const
{
    auto lat = latitude();
    auto lon = longitude();
    return lat.has_value() && lon.has_value() &&
           lat.value() >= -90.0 && lat.value() <= 90.0 &&
           lon.value() >= -180.0 && lon.value() <= 180.0;
}

void CameraGeolocationWidget::onCoordinateChanged()
{
    updateValidationState();
    emit coordinatesChanged();
}

void CameraGeolocationWidget::onAddressChanged()
{
    emit addressChanged();
}

void CameraGeolocationWidget::onDetectGpsLocation()
{
    auto source = QGeoPositionInfoSource::createDefaultSource(this);
    if (!source)
    {
        m_validationLabel->setText(tr("GPS not available on this device"));
        m_validationLabel->show();
        return;
    }

    connect(source, &QGeoPositionInfoSource::positionUpdated,
        [this, source](const QGeoPositionInfo& info)
        {
            if (info.isValid())
            {
                auto coord = info.coordinate();
                setLatitude(coord.latitude());
                setLongitude(coord.longitude());
                if (coord.altitude() != coord.altitude()) // Check for NaN
                    setAltitude(std::nullopt);
                else
                    setAltitude(coord.altitude());
                
                m_validationLabel->hide();
                emit coordinatesChanged();
            }
            source->deleteLater();
        });

    connect(source, &QGeoPositionInfoSource::errorOccurred,
        [this, source](QGeoPositionInfoSource::Error error)
        {
            Q_UNUSED(error)
            m_validationLabel->setText(tr("Failed to detect GPS location"));
            m_validationLabel->show();
            source->deleteLater();
        });

    source->requestUpdate(30000); // 30 second timeout
}

void CameraGeolocationWidget::onClearLocation()
{
    clearLocation();
    emit locationCleared();
}

void CameraGeolocationWidget::onOpenMap()
{
    emit openMapRequested();
}

void CameraGeolocationWidget::updateValidationState()
{
    auto lat = latitude();
    auto lon = longitude();
    
    if (lat.has_value() && lon.has_value())
    {
        if (hasValidCoordinates())
        {
            m_validationLabel->hide();
        }
        else
        {
            m_validationLabel->setText(tr("Invalid coordinates"));
            m_validationLabel->show();
        }
    }
    else if (lat.has_value() || lon.has_value())
    {
        m_validationLabel->setText(tr("Both latitude and longitude are required"));
        m_validationLabel->show();
    }
    else
    {
        m_validationLabel->hide();
    }
}

} // namespace nx::vms::client::desktop
