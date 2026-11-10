// ============================================================================
//  HistoryPage.cpp  –  Reservation history (doubly-linked list traversal)
// ============================================================================
#include "HistoryPage.h"
#include "../services/AppContext.h"
#include "../core/ParkingTypes.h"
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QPushButton>

HistoryPage::HistoryPage(AppContext* ctx, QWidget* parent)
    : QWidget(parent), m_ctx(ctx)
{
    buildUi();
    refresh();
}

void HistoryPage::buildUi() {
    QVBoxLayout* root = new QVBoxLayout(this);
    root->setContentsMargins(24, 24, 24, 24);
    root->setSpacing(16);

    QLabel* title = new QLabel("Reservation History");
    title->setObjectName("pageTitle");
    QLabel* sub = new QLabel(
        "Browse history using the doubly-linked list (oldest-first or newest-first)");
    sub->setObjectName("pageSubtitle");
    root->addWidget(title);
    root->addWidget(sub);

    // Controls row
    QHBoxLayout* ctrlRow = new QHBoxLayout;
    QLabel* plateLbl = new QLabel("Vehicle Plate:");
    plateLbl->setObjectName("formLabel");
    m_plateSearch = new QLineEdit;
    m_plateSearch->setPlaceholderText("e.g. UK07CA2222  (leave blank for your plate)");
    m_plateSearch->setMaximumWidth(280);

    m_directionCombo = new QComboBox;
    m_directionCombo->addItem("Oldest first (forward →)",  true);
    m_directionCombo->addItem("Newest first (← reverse)", false);

    QPushButton* searchBtn = new QPushButton("Search");
    searchBtn->setObjectName("primaryBtn");
    connect(searchBtn, &QPushButton::clicked, this, &HistoryPage::onSearch);
    connect(m_directionCombo,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &HistoryPage::onDirectionChanged);

    ctrlRow->addWidget(plateLbl);
    ctrlRow->addWidget(m_plateSearch);
    ctrlRow->addWidget(m_directionCombo);
    ctrlRow->addWidget(searchBtn);
    ctrlRow->addStretch();
    root->addLayout(ctrlRow);

    // History table
    m_histTable = new QTableWidget(0, 5);
    m_histTable->setHorizontalHeaderLabels(
        {"Res #", "Plate", "Slot", "Action", "Time"});
    m_histTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_histTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_histTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_histTable->setAlternatingRowColors(true);
    m_histTable->verticalHeader()->hide();
    root->addWidget(m_histTable, 1);
}

void HistoryPage::refresh() {
    // Default: show current user's history, newest first
    if (m_ctx->isLoggedIn && m_plateSearch->text().trimmed().isEmpty())
        m_plateSearch->setText(QString::fromStdString(m_ctx->currentUser.plate));
    onSearch();
}

void HistoryPage::onSearch() {
    QString plate   = m_plateSearch->text().trimmed().toUpper();
    bool    forward = m_directionCombo->currentData().toBool();
    if (plate.isEmpty() && m_ctx->isLoggedIn)
        plate = QString::fromStdString(m_ctx->currentUser.plate);
    populateTable(plate, forward);
}

void HistoryPage::onDirectionChanged() { onSearch(); }

void HistoryPage::populateTable(const QString& plate, bool forward) {
    m_histTable->setRowCount(0);
    if (plate.isEmpty()) return;

    auto records = m_ctx->parking.getHistoryFor(plate.toStdString(), forward);
    for (auto& r : records) {
        int row = m_histTable->rowCount();
        m_histTable->insertRow(row);
        m_histTable->setItem(row, 0, new QTableWidgetItem(
            QString::number(r.resId)));
        m_histTable->setItem(row, 1, new QTableWidgetItem(
            QString::fromStdString(r.plate)));
        m_histTable->setItem(row, 2, new QTableWidgetItem(
            QString("S%1").arg(r.slotId)));

        QTableWidgetItem* actionItem = new QTableWidgetItem(
            QString::fromStdString(r.action));
        if (r.action == "RESERVED")
            actionItem->setForeground(QColor("#27AE60"));
        else if (r.action == "CANCELLED")
            actionItem->setForeground(QColor("#E74C3C"));
        else if (r.action == "UNDONE")
            actionItem->setForeground(QColor("#F39C12"));
        m_histTable->setItem(row, 3, actionItem);

        m_histTable->setItem(row, 4, new QTableWidgetItem(
            QString::fromStdString(r.time)));
    }
    if (records.empty()) {
        m_histTable->insertRow(0);
        QTableWidgetItem* noData = new QTableWidgetItem("No history found for " + plate);
        noData->setForeground(QColor("#9CA3AF"));
        m_histTable->setItem(0, 0, noData);
    }
}
