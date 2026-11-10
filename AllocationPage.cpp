// ============================================================================
//  AllocationPage.cpp  –  Priority-based slot allocation form
// ============================================================================
#include "AllocationPage.h"
#include "../services/AppContext.h"
#include "../core/ParkingTypes.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QMessageBox>
#include <QPushButton>
#include <QFrame>

AllocationPage::AllocationPage(AppContext* ctx, QWidget* parent)
    : QWidget(parent), m_ctx(ctx)
{
    buildUi();
    refresh();
}

void AllocationPage::buildUi() {
    QVBoxLayout* root = new QVBoxLayout(this);
    root->setContentsMargins(24, 24, 24, 24);
    root->setSpacing(16);

    QLabel* title = new QLabel("Priority-Based Parking Allocation");
    title->setObjectName("pageTitle");
    QLabel* sub   = new QLabel(
        "The allocation algorithm ranks slots using distance, area occupancy "
        "and vehicle compatibility (Dijkstra + scored priority queue).");
    sub->setObjectName("pageSubtitle");
    sub->setWordWrap(true);
    root->addWidget(title);
    root->addWidget(sub);

    QHBoxLayout* mainRow = new QHBoxLayout;
    mainRow->setSpacing(20);

    // ── Left: Input form ─────────────────────────────────────────────────────
    QGroupBox* formBox = new QGroupBox("Vehicle & Preference Details");
    formBox->setMaximumWidth(380);
    QFormLayout* form = new QFormLayout(formBox);
    form->setSpacing(12);
    form->setContentsMargins(16, 20, 16, 16);

    m_plateLine = new QLineEdit;
    m_plateLine->setPlaceholderText("e.g. UK07CA2222");
    m_nameLine  = new QLineEdit;
    m_nameLine->setPlaceholderText("Owner's name");

    m_vehicleCombo = new QComboBox;
    m_vehicleCombo->addItem("Bike / Compact", 1);
    m_vehicleCombo->addItem("Car",            2);
    m_vehicleCombo->addItem("Electric Vehicle (EV)", 3);
    m_vehicleCombo->addItem("Accessible (permit)",   4);

    m_prefCombo = new QComboBox;
    m_prefCombo->addItem("Closest to entrance",  1);
    m_prefCombo->addItem("Least crowded area",   2);
    m_prefCombo->addItem("Balanced (recommended)", 3);
    m_prefCombo->setCurrentIndex(2);

    m_coveredCheck = new QCheckBox("Prefer covered parking");

    form->addRow("Vehicle Plate:",   m_plateLine);
    form->addRow("Owner Name:",      m_nameLine);
    form->addRow("Vehicle Type:",    m_vehicleCombo);
    form->addRow("Preference:",      m_prefCombo);
    form->addRow("",                 m_coveredCheck);

    // Buttons
    QPushButton* findBtn = new QPushButton("🔍  Find Best Slots");
    findBtn->setObjectName("primaryBtn");
    findBtn->setFixedHeight(42);
    connect(findBtn, &QPushButton::clicked, this, &AllocationPage::onFindSlots);

    QPushButton* allocBtn = new QPushButton("✅  Allocate Selected Slot");
    allocBtn->setObjectName("successBtn");
    allocBtn->setFixedHeight(42);
    connect(allocBtn, &QPushButton::clicked, this, &AllocationPage::onAllocate);

    QPushButton* cancelBtn = new QPushButton("❌  Cancel My Reservation");
    cancelBtn->setObjectName("dangerBtn");
    cancelBtn->setFixedHeight(42);
    connect(cancelBtn, &QPushButton::clicked, this, &AllocationPage::onCancel);

    QPushButton* undoBtn = new QPushButton("↩  Undo Last Reservation");
    undoBtn->setObjectName("secondaryBtn");
    undoBtn->setFixedHeight(42);
    connect(undoBtn, &QPushButton::clicked, this, &AllocationPage::onUndo);

    QPushButton* waitBtn = new QPushButton("⏳  Join Waiting List");
    waitBtn->setObjectName("secondaryBtn");
    waitBtn->setFixedHeight(42);
    connect(waitBtn, &QPushButton::clicked, this, &AllocationPage::onJoinWaitlist);

    QVBoxLayout* btnCol = new QVBoxLayout;
    btnCol->setSpacing(8);
    btnCol->addWidget(findBtn);
    btnCol->addWidget(allocBtn);
    btnCol->addWidget(cancelBtn);
    btnCol->addWidget(undoBtn);
    btnCol->addWidget(waitBtn);

    QVBoxLayout* leftVlay = new QVBoxLayout;
    leftVlay->addWidget(formBox);
    leftVlay->addLayout(btnCol);
    leftVlay->addStretch();

    // ── Right: Results ────────────────────────────────────────────────────────
    QVBoxLayout* rightVlay = new QVBoxLayout;

    m_statusLabel = new QLabel;
    m_statusLabel->setWordWrap(true);
    m_statusLabel->setObjectName("pageSubtitle");

    QLabel* rankTitle = new QLabel("Ranked Slot Candidates");
    rankTitle->setObjectName("pageTitle");
    rankTitle->setStyleSheet("font-size: 15px;");

    QLabel* rankHint = new QLabel("Click a row then press 'Allocate Selected Slot'");
    rankHint->setObjectName("pageSubtitle");

    m_candidateTable = new QTableWidget(0, 6);
    m_candidateTable->setHorizontalHeaderLabels(
        {"#", "Slot", "Area", "Type", "Dist (m)", "Score"});
    m_candidateTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_candidateTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_candidateTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_candidateTable->setAlternatingRowColors(true);
    m_candidateTable->verticalHeader()->hide();
    m_candidateTable->setMaximumHeight(200);

    m_resultLabel = new QLabel;
    m_resultLabel->setObjectName("successMsg");
    m_resultLabel->setWordWrap(true);

    rightVlay->addWidget(m_statusLabel);
    rightVlay->addSpacing(8);
    rightVlay->addWidget(rankTitle);
    rightVlay->addWidget(rankHint);
    rightVlay->addWidget(m_candidateTable);
    rightVlay->addWidget(m_resultLabel);
    rightVlay->addStretch();

    mainRow->addLayout(leftVlay, 1);
    mainRow->addLayout(rightVlay, 2);
    root->addLayout(mainRow, 1);
}

// ---------------------------------------------------------------------------
//  refresh  --  pre-fill form with current user info
// ---------------------------------------------------------------------------
void AllocationPage::refresh() {
    if (m_ctx->isLoggedIn) {
        m_plateLine->setText(QString::fromStdString(m_ctx->currentUser.plate));
        m_nameLine->setText(QString::fromStdString(m_ctx->currentUser.name));
        int vIdx = m_vehicleCombo->findData(m_ctx->currentUser.vehicle);
        if (vIdx >= 0) m_vehicleCombo->setCurrentIndex(vIdx);
    }
    updateStatus();
    m_resultLabel->clear();
}

void AllocationPage::updateStatus() {
    if (!m_ctx->isLoggedIn) return;
    std::string plate = m_ctx->currentUser.plate;
    const Reservation* r = m_ctx->parking.getActiveReservation(plate);
    if (r) {
        m_statusLabel->setText(QString(
            "ℹ️  You have an active reservation: <b>Res #%1</b> for Slot <b>S%2</b> "
            "(booked at %3).")
            .arg(r->id).arg(r->slotId)
            .arg(QString::fromStdString(r->time)));
        m_statusLabel->setStyleSheet("color: #27AE60; font-size: 13px;");
    } else {
        int pos = m_ctx->parking.waitPositionFor(plate);
        if (pos > 0) {
            m_statusLabel->setText(QString(
                "⏳  You are on the waiting list at position <b>%1</b>.").arg(pos));
            m_statusLabel->setStyleSheet("color: #F39C12; font-size: 13px;");
        } else {
            m_statusLabel->setText("No active reservation.");
            m_statusLabel->setStyleSheet("color: #7F8C8D; font-size: 13px;");
        }
    }
}

// ---------------------------------------------------------------------------
//  onFindSlots  --  rank candidates using existing priority algorithm
// ---------------------------------------------------------------------------
void AllocationPage::onFindSlots() {
    m_resultLabel->clear();
    m_candidateTable->setRowCount(0);
    m_candidates.clear();

    QString plate = m_plateLine->text().trimmed().toUpper();
    QString name  = m_nameLine->text().trimmed();
    int vType     = m_vehicleCombo->currentData().toInt();

    if (plate.isEmpty() || name.isEmpty()) {
        m_resultLabel->setStyleSheet("color: #E74C3C;");
        m_resultLabel->setText("Please enter a vehicle plate and owner name.");
        return;
    }

    Pref pref;
    pref.mode    = m_prefCombo->currentData().toInt();
    pref.covered = m_coveredCheck->isChecked();

    // Call the existing ranking algorithm
    auto rawCands = m_ctx->parking.getRankedCandidates(vType, pref, 5);
    if (rawCands.empty()) {
        m_resultLabel->setStyleSheet("color: #E74C3C;");
        m_resultLabel->setText(
            "No suitable slots available for this vehicle type. "
            "Consider joining the waiting list.");
        return;
    }

    auto allSlots = m_ctx->parking.getSlots();
    for (auto& c : rawCands) {
        const Slot& s = allSlots[c.slotIdx];
        m_candidates.append({c.score, c.slotIdx});

        int row = m_candidateTable->rowCount();
        m_candidateTable->insertRow(row);
        m_candidateTable->setItem(row, 0, new QTableWidgetItem(
            QString::number(row + 1)));
        m_candidateTable->setItem(row, 1, new QTableWidgetItem(
            QString("S%1").arg(s.id)));
        m_candidateTable->setItem(row, 2, new QTableWidgetItem(
            QString(s.area)));
        QString typeName;
        switch (s.type) {
            case S_COMPACT: typeName = "Compact";     break;
            case S_REGULAR: typeName = "Regular";     break;
            case S_EV:      typeName = "EV-Charging"; break;
            case S_ACCESS:  typeName = "Accessible";  break;
        }
        m_candidateTable->setItem(row, 3, new QTableWidgetItem(typeName +
            (s.covered ? " ☂" : "")));
        m_candidateTable->setItem(row, 4, new QTableWidgetItem(
            QString::number(s.dist)));
        m_candidateTable->setItem(row, 5, new QTableWidgetItem(
            QString::number(c.score, 'f', 3)));
    }
    m_candidateTable->selectRow(0);  // best slot pre-selected
    m_resultLabel->setStyleSheet("color: #2563EB;");
    m_resultLabel->setText(
        QString("Found %1 compatible slot(s). Select one and click Allocate.")
        .arg(rawCands.size()));
}

// ---------------------------------------------------------------------------
//  onAllocate  --  reserve the selected candidate slot
// ---------------------------------------------------------------------------
void AllocationPage::onAllocate() {
    if (m_candidates.isEmpty()) {
        QMessageBox::information(this, "No Candidates",
            "Please click 'Find Best Slots' first.");
        return;
    }
    int selRow = m_candidateTable->currentRow();
    if (selRow < 0 || selRow >= m_candidates.size()) {
        QMessageBox::information(this, "No Row Selected",
            "Please select a row from the candidates table.");
        return;
    }

    QString plate = m_plateLine->text().trimmed().toUpper();
    QString name  = m_nameLine->text().trimmed();
    int vType     = m_vehicleCombo->currentData().toInt();

    Person p;
    p.plate   = plate.toStdString();
    p.name    = name.toStdString();
    p.vehicle = vType;

    int slotIdx = m_candidates[selRow].slotIdx;
    int rid     = m_ctx->parking.reserve(p, slotIdx);

    if (rid == -2) {
        m_resultLabel->setStyleSheet("color: #E74C3C;");
        m_resultLabel->setText("This vehicle already has an active reservation.");
        return;
    }
    if (rid < 0) {
        m_resultLabel->setStyleSheet("color: #E74C3C;");
        m_resultLabel->setText("Could not allocate slot (it may now be occupied).");
        return;
    }

    Slot s = m_ctx->parking.getSlotById(m_ctx->parking.getSlots()[slotIdx].id);
    m_resultLabel->setStyleSheet("color: #27AE60;");
    m_resultLabel->setText(QString(
        "✅ Reservation <b>#%1</b> confirmed!<br>"
        "Slot <b>S%2</b> (Area %3) allocated to <b>%4</b>.")
        .arg(rid).arg(s.id).arg(s.area).arg(name));

    emit dataChanged();
    refresh();
}

// ---------------------------------------------------------------------------
//  onCancel  --  cancel the current user's reservation
// ---------------------------------------------------------------------------
void AllocationPage::onCancel() {
    QString plate = m_plateLine->text().trimmed().toUpper();
    if (plate.isEmpty()) {
        QMessageBox::warning(this, "Error", "Enter a vehicle plate first.");
        return;
    }
    auto r = m_ctx->parking.getActiveReservation(plate.toStdString());
    if (!r) {
        QMessageBox::information(this, "No Reservation",
            "No active reservation found for " + plate + ".");
        return;
    }
    auto ans = QMessageBox::question(this, "Cancel Reservation",
        QString("Cancel Reservation #%1 for Slot S%2?").arg(r->id).arg(r->slotId),
        QMessageBox::Yes | QMessageBox::No);
    if (ans == QMessageBox::Yes) {
        m_ctx->parking.cancelByPlate(plate.toStdString());
        m_resultLabel->setStyleSheet("color: #E74C3C;");
        m_resultLabel->setText("Reservation cancelled.");
        emit dataChanged();
        refresh();
    }
}

// ---------------------------------------------------------------------------
//  onUndo  --  undo last reservation (stack-based)
// ---------------------------------------------------------------------------
void AllocationPage::onUndo() {
    QString plate = m_plateLine->text().trimmed().toUpper();
    if (plate.isEmpty()) {
        QMessageBox::warning(this, "Error", "Enter a vehicle plate first.");
        return;
    }
    if (m_ctx->parking.undoLast(plate.toStdString())) {
        m_resultLabel->setStyleSheet("color: #F39C12;");
        m_resultLabel->setText("↩ Last reservation undone.");
        emit dataChanged();
        refresh();
    } else {
        m_resultLabel->setStyleSheet("color: #6B7280;");
        m_resultLabel->setText("Nothing to undo for this vehicle.");
    }
}

// ---------------------------------------------------------------------------
//  onJoinWaitlist
// ---------------------------------------------------------------------------
void AllocationPage::onJoinWaitlist() {
    QString plate = m_plateLine->text().trimmed().toUpper();
    QString name  = m_nameLine->text().trimmed();
    int vType     = m_vehicleCombo->currentData().toInt();
    if (plate.isEmpty() || name.isEmpty()) {
        QMessageBox::warning(this, "Error", "Enter plate and name first.");
        return;
    }
    Person p{plate.toStdString(), name.toStdString(), vType};
    Pref pref;
    pref.mode    = m_prefCombo->currentData().toInt();
    pref.covered = m_coveredCheck->isChecked();
    if (m_ctx->parking.joinWaitList(p, pref)) {
        int pos = m_ctx->parking.waitPositionFor(plate.toStdString());
        m_resultLabel->setStyleSheet("color: #8B5CF6;");
        m_resultLabel->setText(QString(
            "⏳ Added to waiting list at position %1.").arg(pos));
        emit dataChanged();
        updateStatus();
    } else {
        m_resultLabel->setStyleSheet("color: #E74C3C;");
        m_resultLabel->setText("Already on the waiting list.");
    }
}
