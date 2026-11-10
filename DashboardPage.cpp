// ============================================================================
//  DashboardPage.cpp
// ============================================================================
#include "DashboardPage.h"
#include "../services/AppContext.h"
#include "../core/ParkingTypes.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QScrollArea>
#include <QHeaderView>
#include <QDateTime>
#include <ctime>

DashboardPage::DashboardPage(AppContext* ctx, QWidget* parent)
    : QWidget(parent), m_ctx(ctx)
{
    buildUi();
    refresh();
}

// ---------------------------------------------------------------------------
//  Helper: create one stat card widget
// ---------------------------------------------------------------------------
QWidget* DashboardPage::makeStatCard(const QString& icon, const QString& title,
                                      QLabel*& valueLabel, const QString& colorHex) {
    QWidget* card = new QWidget;
    card->setObjectName("statCard");

    QHBoxLayout* h = new QHBoxLayout(card);
    h->setContentsMargins(16, 16, 16, 16);

    // Left: value + label
    QVBoxLayout* info = new QVBoxLayout;
    valueLabel = new QLabel("0");
    valueLabel->setObjectName("statValue");
    valueLabel->setStyleSheet("color: " + colorHex + "; font-size: 30px; font-weight: bold;");

    QLabel* lbl = new QLabel(title);
    lbl->setObjectName("statLabel");

    info->addWidget(valueLabel);
    info->addWidget(lbl);

    // Right: icon
    QLabel* ico = new QLabel(icon);
    ico->setObjectName("statIcon");
    ico->setStyleSheet("font-size: 28px;");
    ico->setAlignment(Qt::AlignCenter);

    h->addLayout(info);
    h->addStretch();
    h->addWidget(ico);

    card->setMinimumSize(160, 90);
    return card;
}

// ---------------------------------------------------------------------------
//  buildUi
// ---------------------------------------------------------------------------
void DashboardPage::buildUi() {
    QVBoxLayout* root = new QVBoxLayout(this);
    root->setContentsMargins(24, 24, 24, 24);
    root->setSpacing(20);

    // Page header
    QLabel* title = new QLabel("Dashboard");
    title->setObjectName("pageTitle");
    QLabel* sub   = new QLabel("Live overview of ParkNexa operations");
    sub->setObjectName("pageSubtitle");
    root->addWidget(title);
    root->addWidget(sub);

    // ── Stat cards grid ──────────────────────────────────────────────────────
    QGridLayout* grid = new QGridLayout;
    grid->setSpacing(14);
    grid->addWidget(makeStatCard("🅿",  "Total Slots",     m_totalLabel,    "#1A2744"), 0, 0);
    grid->addWidget(makeStatCard("✅",  "Available",       m_freeLabel,     "#27AE60"), 0, 1);
    grid->addWidget(makeStatCard("🚗",  "Occupied",        m_occupiedLabel, "#E74C3C"), 0, 2);
    grid->addWidget(makeStatCard("📌",  "Reserved",        m_reservedLabel, "#F39C12"), 0, 3);
    grid->addWidget(makeStatCard("⏳",  "Waiting List",    m_waitLabel,     "#8B5CF6"), 1, 0);
    grid->addWidget(makeStatCard("💰",  "Total Revenue",   m_revenueLabel,  "#27AE60"), 1, 1);

    // Area occupancy cards
    QWidget* areaCard = new QWidget;
    areaCard->setObjectName("statCard");
    QVBoxLayout* aVlay = new QVBoxLayout(areaCard);
    QLabel* aTitle = new QLabel("Area Occupancy");
    aTitle->setObjectName("statLabel");
    aVlay->addWidget(aTitle);
    QHBoxLayout* aH = new QHBoxLayout;
    m_areaA = new QLabel("A: --%"); m_areaA->setObjectName("statLabel");
    m_areaB = new QLabel("B: --%"); m_areaB->setObjectName("statLabel");
    m_areaC = new QLabel("C: --%"); m_areaC->setObjectName("statLabel");
    aH->addWidget(m_areaA); aH->addWidget(m_areaB); aH->addWidget(m_areaC);
    aVlay->addLayout(aH);
    grid->addWidget(areaCard, 1, 2, 1, 2);

    root->addLayout(grid);

    // ── Recent Reservations table ─────────────────────────────────────────────
    QLabel* rTitle = new QLabel("Recent Reservations");
    rTitle->setObjectName("pageTitle");
    rTitle->setStyleSheet("font-size: 16px;");
    root->addWidget(rTitle);

    m_recentTable = new QTableWidget(0, 5);
    m_recentTable->setHorizontalHeaderLabels(
        {"Res #", "Vehicle Plate", "Owner", "Slot", "Time / Status"});
    m_recentTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_recentTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_recentTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_recentTable->setAlternatingRowColors(true);
    m_recentTable->verticalHeader()->hide();
    m_recentTable->setMaximumHeight(280);
    root->addWidget(m_recentTable);
    root->addStretch();
}

// ---------------------------------------------------------------------------
//  refresh  --  pull live data from the engines and update all widgets
// ---------------------------------------------------------------------------
void DashboardPage::refresh() {
    // Slot stats
    m_totalLabel->setText(QString::number(m_ctx->parking.totalSlots()));
    m_freeLabel->setText(QString::number(m_ctx->parking.freeSlots()));
    m_occupiedLabel->setText(QString::number(m_ctx->parking.occupiedSlots()));
    m_reservedLabel->setText(QString::number(m_ctx->parking.reservedSlots()));
    m_waitLabel->setText(QString::number(m_ctx->parking.waitListSize()));

    // Revenue
    double rev = m_ctx->charges.totalRevenue();
    m_revenueLabel->setText(QString("Rs. %1").arg(rev, 0, 'f', 2));

    // Area occupancy
    m_areaA->setText(QString("A: %1%").arg(
        m_ctx->parking.areaOccupancyPct('A'), 0, 'f', 0));
    m_areaB->setText(QString("B: %1%").arg(
        m_ctx->parking.areaOccupancyPct('B'), 0, 'f', 0));
    m_areaC->setText(QString("C: %1%").arg(
        m_ctx->parking.areaOccupancyPct('C'), 0, 'f', 0));

    // Recent reservations table
    auto reservations = m_ctx->parking.getAllReservations();
    m_recentTable->setRowCount(0);
    int limit = std::min(static_cast<int>(reservations.size()), 20);
    for (int i = 0; i < limit; i++) {
        const Reservation& r = reservations[i];
        int row = m_recentTable->rowCount();
        m_recentTable->insertRow(row);
        m_recentTable->setItem(row, 0, new QTableWidgetItem(QString::number(r.id)));
        m_recentTable->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(r.plate)));
        m_recentTable->setItem(row, 2, new QTableWidgetItem(QString::fromStdString(r.owner)));
        m_recentTable->setItem(row, 3, new QTableWidgetItem(QString("S%1").arg(r.slotId)));

        QString status = r.active ? "Active" : "Completed/Cancelled";
        QTableWidgetItem* statusItem = new QTableWidgetItem(
            QString::fromStdString(r.time) + "  [" + status + "]");
        statusItem->setForeground(r.active ? QColor("#27AE60") : QColor("#7F8C8D"));
        m_recentTable->setItem(row, 4, statusItem);
    }
}
