// ============================================================================
//  ChargesPage.h  –  Parking charges: start/end session, view bill, history
// ============================================================================
#pragma once
#include <QWidget>
#include <QComboBox>
#include <QLabel>
#include <QTableWidget>
#include <QFrame>

class AppContext;

class ChargesPage : public QWidget {
    Q_OBJECT
public:
    explicit ChargesPage(AppContext* ctx, QWidget* parent = nullptr);
    void refresh();

signals:
    void dataChanged();

private slots:
    void onStartParking();
    void onEndParking();
    void onSessionSelected(int row, int col);

private:
    AppContext* m_ctx;

    // Session control
    QComboBox*  m_vehicleCombo;
    QLabel*     m_sessionStatus;

    // Bill display area
    QFrame*     m_billCard;
    QLabel*     m_billSessionId;
    QLabel*     m_billPlate;
    QLabel*     m_billEntry;
    QLabel*     m_billExit;
    QLabel*     m_billDuration;
    QLabel*     m_billRate;
    QLabel*     m_billTotal;

    // History table
    QTableWidget* m_historyTable;

    void buildUi();
    void populateVehicleCombo();
    void populateHistory();
    void showBill(const struct Bill& bill);
    void hideBill();
};
