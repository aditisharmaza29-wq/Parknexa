// ============================================================================
//  ParkingPage.cpp  –  Visual parking grid
// ============================================================================
#include "ParkingPage.h"
#include "../services/AppContext.h"
#include "../core/ParkingTypes.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QScrollArea>
#include <QHeaderView>
#include <QGroupBox>
#include <QMessageBox>
#include <QPushButton>
#include <QFrame>
#include <functional>

ParkingPage::ParkingPage(AppContext* ctx, QWidget* parent)
    : QWidget(parent), m_ctx(ctx)
{
    buildUi();
    refresh();
}

// ---------------------------------------------------------------------------
//  buildUi
// ---------------------------------------------------------------------------
void ParkingPage::buildUi() {
    QVBoxLayout* root = new QVBoxLayout(this);
    root->setContentsMargins(24, 24, 24, 24);
    root->setSpacing(16);

    // Header
    QLabel* title = new QLabel("Parking Slot Management");
    title->setObjectName("pageTitle");
    QLabel* sub = new QLabel("Visual layout of all parking slots");
    sub->setObjectName("pageSubtitle");
    root->addWidget(title);
    root->addWidget(sub);

    // Legend
    QHBoxLayout* legend = new QHBoxLayout;
    auto addLegend = [&](const QString& color, const QString& text) {
        QLabel* dot = new QLabel("●");
        dot->setStyleSheet("color: " + color + "; font-size: 18px;");
        QLabel* lbl = new QLabel(text);
        lbl->setStyleSheet("color: #4B5563; font-size: 12px; margin-right: 16px;");
        legend->addWidget(dot); legend->addWidget(lbl);
    };
    addLegend("#27AE60", "Available");
    addLegend("#F39C12", "Reserved");
    addLegend("#E74C3C", "Occupied (walk-in)");
    legend->addStretch();
    root->addLayout(legend);

    // Search bar
    QHBoxLayout* searchRow = new QHBoxLayout;
    m_searchBox = new QLineEdit;
    m_searchBox->setPlaceholderText("Search by slot ID, area, type…");
    m_searchBox->setMaximumWidth(300);
    QPushButton* searchBtn = new QPushButton("Search");
    searchBtn->setObjectName("secondaryBtn");
    QPushButton* clearBtn = new QPushButton("Show All");
    clearBtn->setObjectName("secondaryBtn");
    connect(searchBtn, &QPushButton::clicked, this, &ParkingPage::onSearch);
    connect(clearBtn, &QPushButton::clicked, [this]{ m_searchBox->clear(); rebuildGrid(); });
    searchRow->addWidget(m_searchBox);
    searchRow->addWidget(searchBtn);
    searchRow->addWidget(clearBtn);
    searchRow->addStretch();
    root->addLayout(searchRow);

    // Slot grid (inside a scroll area)
    QScrollArea* scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setObjectName("card");

    m_gridContainer = new QWidget;
    scroll->setWidget(m_gridContainer);
    root->addWidget(scroll, 2);

    // Active reservations table
    QLabel* actTitle = new QLabel("Active Reservations");
    actTitle->setObjectName("pageTitle");
    actTitle->setStyleSheet("font-size: 15px;");
    root->addWidget(actTitle);

    m_activeTable = new QTableWidget(0, 5);
    m_activeTable->setHorizontalHeaderLabels(
        {"Res #", "Plate", "Owner", "Slot", "Time"});
    m_activeTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_activeTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_activeTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_activeTable->setAlternatingRowColors(true);
    m_activeTable->verticalHeader()->hide();
    m_activeTable->setMaximumHeight(180);
    root->addWidget(m_activeTable, 1);
}

// ---------------------------------------------------------------------------
//  makeSlotCard  --  one coloured card widget for a single parking slot
// ---------------------------------------------------------------------------
QWidget* ParkingPage::makeSlotCard(const Slot& slot) {
    QFrame* card = new QFrame;
    card->setObjectName("slotCard");
    card->setFrameShape(QFrame::StyledPanel);
    card->setCursor(Qt::PointingHandCursor);

    QString statusStr;
    QString bgColor, borderColor;
    if (!slot.occupied) {
        statusStr   = "FREE";
        bgColor     = "#EAFAF1";
        borderColor = "#27AE60";
    } else if (slot.resId != 0) {
        statusStr   = "RESERVED";
        bgColor     = "#FEF9E7";
        borderColor = "#F39C12";
    } else {
        statusStr   = "OCCUPIED";
        bgColor     = "#FDEDEC";
        borderColor = "#E74C3C";
    }

    card->setStyleSheet(QString(
        "QFrame { background: %1; border: 2px solid %2; "
        "border-radius: 10px; } "
        "QFrame:hover { border-width: 3px; }")
        .arg(bgColor, borderColor));
    card->setFixedSize(115, 100);

    QVBoxLayout* lay = new QVBoxLayout(card);
    lay->setContentsMargins(10, 10, 10, 10);
    lay->setSpacing(4);

    // Slot ID
    QLabel* idLbl = new QLabel(QString("S%1").arg(slot.id));
    idLbl->setObjectName("slotId");
    idLbl->setStyleSheet("font-weight: bold; font-size: 16px; color: #1A2744;");

    // Type + area
    QString typeName;
    switch (slot.type) {
        case S_COMPACT: typeName = "Compact"; break;
        case S_REGULAR: typeName = "Regular"; break;
        case S_EV:      typeName = "EV";      break;
        case S_ACCESS:  typeName = "Access";  break;
        default:        typeName = "?";
    }
    QLabel* meta = new QLabel(QString("Area %1 · %2%3")
        .arg(slot.area).arg(typeName).arg(slot.covered ? " ☂" : ""));
    meta->setObjectName("slotMeta");
    meta->setStyleSheet("color: #6B7280; font-size: 10px;");

    // Distance
    QLabel* dist = new QLabel(QString("%1m from entrance").arg(slot.dist));
    dist->setStyleSheet("color: #9CA3AF; font-size: 9px;");

    // Status badge
    QLabel* status = new QLabel(statusStr);
    status->setStyleSheet(QString(
        "color: %1; font-weight: bold; font-size: 10px;").arg(borderColor));

    lay->addWidget(idLbl);
    lay->addWidget(meta);
    lay->addWidget(dist);
    lay->addWidget(status);

    // Click handler: show details + offer action
    int slotId = slot.id;
    QObject::connect(card, &QFrame::destroyed, []{}); // keep alive
    // Use event filter approach via a lambda-connected invisible button
    QPushButton* invisible = new QPushButton(card);
    invisible->setGeometry(0, 0, 115, 100);
    invisible->setStyleSheet("background: transparent; border: none;");
    invisible->raise();
    connect(invisible, &QPushButton::clicked, this, [this, slotId](){
        onSlotClicked(slotId);
    });

    return card;
}

// ---------------------------------------------------------------------------
//  rebuildGrid  --  clear and rebuild the slot card grid
// ---------------------------------------------------------------------------
void ParkingPage::rebuildGrid(const QString& filter) {
    // Remove old grid layout content
    QLayout* old = m_gridContainer->layout();
    if (old) {
        QLayoutItem* item;
        while ((item = old->takeAt(0)) != nullptr) {
            if (item->widget()) item->widget()->deleteLater();
            delete item;
        }
        delete old;
    }

    QGridLayout* grid = new QGridLayout(m_gridContainer);
    grid->setSpacing(12);
    grid->setContentsMargins(16, 16, 16, 16);
    grid->setAlignment(Qt::AlignTop | Qt::AlignLeft);

    auto allSlots = m_ctx->parking.getSlots();
    int col = 0, row = 0;
    const int COLS = 5;

    for (const Slot& slot : allSlots) {
        // Apply filter
        if (!filter.isEmpty()) {
            QString info = QString("S%1 %2 %3").arg(slot.id).arg(slot.area).arg(slot.type);
            if (!info.contains(filter, Qt::CaseInsensitive)) continue;
        }
        grid->addWidget(makeSlotCard(slot), row, col);
        col++;
        if (col >= COLS) { col = 0; row++; }
    }
}

// ---------------------------------------------------------------------------
//  refresh
// ---------------------------------------------------------------------------
void ParkingPage::refresh() {
    rebuildGrid(m_searchBox ? m_searchBox->text().trimmed() : "");

    // Update active reservations table
    m_activeTable->setRowCount(0);
    auto rlist = m_ctx->parking.getAllReservations();
    for (auto& r : rlist) {
        if (!r.active) continue;
        int row = m_activeTable->rowCount();
        m_activeTable->insertRow(row);
        m_activeTable->setItem(row, 0, new QTableWidgetItem(QString::number(r.id)));
        m_activeTable->setItem(row, 1, new QTableWidgetItem(
            QString::fromStdString(r.plate)));
        m_activeTable->setItem(row, 2, new QTableWidgetItem(
            QString::fromStdString(r.owner)));
        m_activeTable->setItem(row, 3, new QTableWidgetItem(
            QString("S%1").arg(r.slotId)));
        m_activeTable->setItem(row, 4, new QTableWidgetItem(
            QString::fromStdString(r.time)));
    }
}

// ---------------------------------------------------------------------------
//  Slots
// ---------------------------------------------------------------------------
void ParkingPage::onSearch() {
    rebuildGrid(m_searchBox->text().trimmed());
}

void ParkingPage::onSlotClicked(int slotId) {
    Slot s = m_ctx->parking.getSlotById(slotId);

    QString typeName;
    switch (s.type) {
        case S_COMPACT: typeName = "Compact";     break;
        case S_REGULAR: typeName = "Regular";     break;
        case S_EV:      typeName = "EV-Charging"; break;
        case S_ACCESS:  typeName = "Accessible";  break;
    }
    QString status = !s.occupied ? "FREE" :
                     (s.resId != 0 ? "RESERVED" : "OCCUPIED (walk-in)");

    QString detail = QString(
        "<b>Slot S%1</b><br>"
        "Area: <b>%2</b>&nbsp;&nbsp;Type: <b>%3</b><br>"
        "Covered: <b>%4</b>&nbsp;&nbsp;Distance: <b>%5 m</b><br>"
        "Status: <b>%6</b><br>")
        .arg(s.id).arg(s.area).arg(typeName)
        .arg(s.covered ? "Yes" : "No").arg(s.dist).arg(status);

    QMessageBox dlg(this);
    dlg.setWindowTitle(QString("Slot S%1 Details").arg(slotId));
    dlg.setText(detail);
    dlg.setTextFormat(Qt::RichText);

    QPushButton* releaseBtn = nullptr;
    if (s.occupied) {
        releaseBtn = dlg.addButton("Release Slot", QMessageBox::ActionRole);
    }
    dlg.addButton(QMessageBox::Close);

    dlg.exec();

    if (releaseBtn && dlg.clickedButton() == releaseBtn) {
        if (m_ctx->parking.releaseSlot(slotId)) {
            QMessageBox::information(this, "Slot Released",
                QString("Slot S%1 has been released.").arg(slotId));
            emit dataChanged();
            refresh();
        } else {
            QMessageBox::warning(this, "Error", "Could not release slot.");
        }
    }
}
