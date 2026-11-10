// ============================================================================
//  ParkingPage.h  –  Visual parking grid + slot detail + allocation
// ============================================================================
#pragma once
#include <QWidget>
#include <QLabel>
#include <QLineEdit>
#include <QTableWidget>
#include <QScrollArea>

class AppContext;

class ParkingPage : public QWidget {
    Q_OBJECT
public:
    explicit ParkingPage(AppContext* ctx, QWidget* parent = nullptr);
    void refresh();

signals:
    void dataChanged();  // emit when a reservation is made/cancelled

private slots:
    void onSearch();
    void onSlotClicked(int slotId);

private:
    AppContext*   m_ctx;
    QWidget*      m_gridContainer;  // holds slot cards
    QLineEdit*    m_searchBox;
    QLabel*       m_detailPanel;    // right-side detail
    QTableWidget* m_activeTable;    // active reservations list

    void buildUi();
    void rebuildGrid(const QString& filter = "");
    QWidget* makeSlotCard(const struct Slot& slot);
};
