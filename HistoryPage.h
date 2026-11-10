// ============================================================================
//  HistoryPage.h  –  Reservation history (doubly-linked list traversal)
// ============================================================================
#pragma once
#include <QWidget>
#include <QTableWidget>
#include <QComboBox>
#include <QLineEdit>

class AppContext;

class HistoryPage : public QWidget {
    Q_OBJECT
public:
    explicit HistoryPage(AppContext* ctx, QWidget* parent = nullptr);
    void refresh();

private slots:
    void onSearch();
    void onDirectionChanged();

private:
    AppContext*   m_ctx;
    QLineEdit*    m_plateSearch;
    QComboBox*    m_directionCombo;
    QTableWidget* m_histTable;

    void buildUi();
    void populateTable(const QString& plate, bool forward);
};
