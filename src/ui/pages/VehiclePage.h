// ============================================================================
//  VehiclePage.h  –  Vehicle management (add, remove, search)
// ============================================================================
#pragma once
#include <QWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QTableWidget>
#include <QLabel>

class AppContext;

class VehiclePage : public QWidget {
    Q_OBJECT
public:
    explicit VehiclePage(AppContext* ctx, QWidget* parent = nullptr);
    void refresh();

private slots:
    void onAddVehicle();
    void onRemoveVehicle();
    void onSearch();

private:
    AppContext*   m_ctx;
    QLineEdit*    m_plateEdit;
    QComboBox*    m_typeCombo;
    QLabel*       m_addError;
    QLineEdit*    m_searchEdit;
    QTableWidget* m_vehicleTable;

    void buildUi();
    void populateTable(const QString& filter = "");
};
