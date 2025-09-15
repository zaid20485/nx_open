// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#pragma once

#include <QtWidgets/QWidget>
#include <QtCore/QScopedPointer>
#include <optional>

class QDoubleSpinBox;
class QLineEdit;
class QPushButton;
class QLabel;

namespace nx::vms::client::desktop {

/**
 * Widget for editing camera geolocation information including coordinates and address.
 * Provides GPS detection functionality and coordinate validation.
 */
class CameraGeolocationWidget: public QWidget
{
    Q_OBJECT

public:
    explicit CameraGeolocationWidget(QWidget* parent = nullptr);
    virtual ~CameraGeolocationWidget();

    // Coordinate getters
    std::optional<double> latitude() const;
    std::optional<double> longitude() const;
    std::optional<double> altitude() const;
    QString geolocationAddress() const;

    // Coordinate setters
    void setLatitude(const std::optional<double>& latitude);
    void setLongitude(const std::optional<double>& longitude);
    void setAltitude(const std::optional<double>& altitude);
    void setGeolocationAddress(const QString& address);

    // Clear all location data
    void clearLocation();

    // Validation
    bool hasValidCoordinates() const;

signals:
    void coordinatesChanged();
    void addressChanged();
    void locationCleared();
    void openMapRequested();

private slots:
    void onCoordinateChanged();
    void onAddressChanged();
    void onDetectGpsLocation();
    void onClearLocation();
    void onOpenMap();

private:
    void setupUi();
    void updateValidationState();

private:
    QDoubleSpinBox* m_latitudeSpinBox;
    QDoubleSpinBox* m_longitudeSpinBox;
    QDoubleSpinBox* m_altitudeSpinBox;
    QLineEdit* m_addressEdit;
    QPushButton* m_detectGpsButton;
    QPushButton* m_clearLocationButton;
    QPushButton* m_openMapButton;
    QLabel* m_validationLabel;
};

} // namespace nx::vms::client::desktop
