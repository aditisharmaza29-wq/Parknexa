// ============================================================================
//  AllocationPage.h  –  Priority-based parking allocation form
// ============================================================================
#pragma once
#include <QWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QLabel>
#include <QTableWidget>
#include <QCheckBox>

class AppContext;

class AllocationPage : public QWidget {
    Q_OBJECT
public:
    explicit AllocationPage(AppContext* ctx, QWidget* parent = nullptr);
    void refresh();

signals:
    void dataChanged();

private slots:
    void onFindSlots();
    void onAllocate();
    void onCancel();
    void onUndo();
    void onJoinWaitlist();

private:
    AppContext*   m_ctx;

    // Input form
    QLineEdit*    m_plateLine;
    QLineEdit*    m_nameLine;
    QComboBox*    m_vehicleCombo;
    QComboBox*    m_prefCombo;
    QCheckBox*    m_coveredCheck;

    // Ranked candidates table
    QTableWidget* m_candidateTable;
    QLabel*       m_resultLabel;
    QLabel*       m_statusLabel;

    // Current candidates (filled by onFindSlots)
    struct Candidate { double score; int slotIdx; };
    QVector<Candidate> m_candidates;

    void buildUi();
    void updateStatus();
};
