// ============================================================================
//  DashboardPage.h  –  Overview stats, occupancy, recent activity
// ============================================================================
#pragma once
#include <QWidget>
#include <QLabel>
#include <QTableWidget>

class AppContext;

class DashboardPage : public QWidget {
    Q_OBJECT
public:
    explicit DashboardPage(AppContext* ctx, QWidget* parent = nullptr);
    void refresh();  // called whenever data changes

private:
    AppContext* m_ctx;

    // Stat card value labels
    QLabel* m_totalLabel;
    QLabel* m_freeLabel;
    QLabel* m_occupiedLabel;
    QLabel* m_reservedLabel;
    QLabel* m_waitLabel;
    QLabel* m_revenueLabel;

    // Area occupancy labels
    QLabel* m_areaA;
    QLabel* m_areaB;
    QLabel* m_areaC;

    // Recent reservations table
    QTableWidget* m_recentTable;

    void buildUi();
    QWidget* makeStatCard(const QString& icon, const QString& title,
                          QLabel*& valueLabel, const QString& colorHex);
};
