// ============================================================================
//  ChargesPage.cpp  –  Parking charges: start/end session, bill, history
// ============================================================================
#include "ChargesPage.h"
#include "../services/AppContext.h"
#include "../core/ParkingTypes.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QPushButton>
#include <QFrame>
#include <ctime>
#include <sstream>
#include <iomanip>

static QString fmtTime(long long t) {
    if (t == 0) return "Active";
    time_t tt = static_cast<time_t>(t);
    tm* local = localtime(&tt);
    if (!local) return "Unknown";
    std::ostringstream oss;
    oss << std::put_time(local, "%d-%m-%Y %I:%M:%S %p");
    return QString::fromStdString(oss.str());
}

ChargesPage::ChargesPage(AppContext* ctx, QWidget* parent)
    : QWidget(parent), m_ctx(ctx)
{
    buildUi();
    refresh();
}

void ChargesPage::buildUi() {
    QVBoxLayout* root = new QVBoxLayout(this);
    root->setContentsMargins(24, 24, 24, 24);
    root->setSpacing(16);

    QLabel* title = new QLabel("Parking Charges & Billing");
    title->setObjectName("pageTitle");
    QLabel* sub = new QLabel(
        "Rate: Rs. 25 flat for ≤30 min · Rs. 50/hour beyond 30 min");
    sub->setObjectName("pageSubtitle");
    root->addWidget(title);
    root->addWidget(sub);

    QHBoxLayout* mainRow = new QHBoxLayout;
    mainRow->setSpacing(20);

    // ── Left: Session control ─────────────────────────────────────────────────
    QGroupBox* sessBox = new QGroupBox("Parking Session");
    sessBox->setMaximumWidth(320);
    QVBoxLayout* sessLay = new QVBoxLayout(sessBox);
    sessLay->setSpacing(12);
    sessLay->setContentsMargins(16, 20, 16, 16);

    QLabel* vLbl = new QLabel("Select Vehicle:");
    vLbl->setObjectName("formLabel");
    m_vehicleCombo = new QComboBox;

    QPushButton* startBtn = new QPushButton("▶  Start Parking");
    startBtn->setObjectName("successBtn");
    startBtn->setFixedHeight(42);
    connect(startBtn, &QPushButton::clicked, this, &ChargesPage::onStartParking);

    QPushButton* endBtn = new QPushButton("⏹  End Parking & Generate Bill");
    endBtn->setObjectName("dangerBtn");
    endBtn->setFixedHeight(42);
    connect(endBtn, &QPushButton::clicked, this, &ChargesPage::onEndParking);

    m_sessionStatus = new QLabel("Select a vehicle above.");
    m_sessionStatus->setObjectName("pageSubtitle");
    m_sessionStatus->setWordWrap(true);

    sessLay->addWidget(vLbl);
    sessLay->addWidget(m_vehicleCombo);
    sessLay->addWidget(m_sessionStatus);
    sessLay->addSpacing(8);
    sessLay->addWidget(startBtn);
    sessLay->addWidget(endBtn);
    sessLay->addStretch();

    // ── Bill card ─────────────────────────────────────────────────────────────
    m_billCard = new QFrame;
    m_billCard->setObjectName("card");
    m_billCard->setMaximumWidth(340);
    QVBoxLayout* billLay = new QVBoxLayout(m_billCard);
    billLay->setSpacing(8);
    billLay->setContentsMargins(20, 20, 20, 20);

    QLabel* billTitle = new QLabel("🧾 Parking Bill");
    billTitle->setStyleSheet("font-size: 16px; font-weight: bold; color: #1A2744;");
    billLay->addWidget(billTitle);

    QFrame* divider = new QFrame;
    divider->setFrameShape(QFrame::HLine);
    divider->setStyleSheet("color: #E8ECF0;");
    billLay->addWidget(divider);

    auto addBillRow = [&](const QString& label, QLabel*& valueLabel) {
        QHBoxLayout* row = new QHBoxLayout;
        QLabel* lbl = new QLabel(label);
        lbl->setObjectName("formLabel");
        lbl->setMinimumWidth(120);
        valueLabel = new QLabel("—");
        row->addWidget(lbl);
        row->addWidget(valueLabel);
        billLay->addLayout(row);
    };

    addBillRow("Session ID:",  m_billSessionId);
    addBillRow("Vehicle:",     m_billPlate);
    addBillRow("Entry Time:",  m_billEntry);
    addBillRow("Exit Time:",   m_billExit);
    addBillRow("Duration:",    m_billDuration);
    addBillRow("Rate:",        m_billRate);

    QFrame* div2 = new QFrame;
    div2->setFrameShape(QFrame::HLine);
    div2->setStyleSheet("color: #E8ECF0;");
    billLay->addWidget(div2);

    m_billTotal = new QLabel("—");
    m_billTotal->setStyleSheet(
        "font-size: 20px; font-weight: bold; color: #27AE60;");
    QLabel* totalLbl = new QLabel("TOTAL CHARGES:");
    totalLbl->setObjectName("formLabel");
    QHBoxLayout* totalRow = new QHBoxLayout;
    totalRow->addWidget(totalLbl);
    totalRow->addWidget(m_billTotal);
    billLay->addLayout(totalRow);
    billLay->addStretch();

    hideBill();

    QVBoxLayout* leftVlay = new QVBoxLayout;
    leftVlay->addWidget(sessBox);
    leftVlay->addWidget(m_billCard);
    leftVlay->addStretch();

    // ── Right: History table ──────────────────────────────────────────────────
    QVBoxLayout* rightVlay = new QVBoxLayout;
    QLabel* histTitle = new QLabel("Parking History");
    histTitle->setObjectName("pageTitle");
    histTitle->setStyleSheet("font-size: 15px;");

    m_historyTable = new QTableWidget(0, 5);
    m_historyTable->setHorizontalHeaderLabels(
        {"Sess #", "Plate", "Entry", "Exit", "Charge (Rs.)"});
    m_historyTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_historyTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_historyTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_historyTable->setAlternatingRowColors(true);
    m_historyTable->verticalHeader()->hide();
    connect(m_historyTable, &QTableWidget::cellClicked,
            this, &ChargesPage::onSessionSelected);

    rightVlay->addWidget(histTitle);
    rightVlay->addWidget(m_historyTable);

    mainRow->addLayout(leftVlay, 1);
    mainRow->addLayout(rightVlay, 2);
    root->addLayout(mainRow, 1);
}

void ChargesPage::refresh() {
    populateVehicleCombo();
    populateHistory();
}

void ChargesPage::populateVehicleCombo() {
    m_vehicleCombo->clear();
    if (!m_ctx->isLoggedIn) return;
    auto vehicles = m_ctx->charges.getVehiclesForUser(m_ctx->currentUser.userId);
    if (vehicles.empty()) {
        m_vehicleCombo->addItem("No vehicles registered");
    } else {
        for (auto& v : vehicles) {
            m_vehicleCombo->addItem(
                QString::fromStdString(v.plate) + "  (" +
                QString::fromStdString(v.type) + ")",
                QString::fromStdString(v.plate));
        }
    }
}

void ChargesPage::populateHistory() {
    m_historyTable->setRowCount(0);
    if (!m_ctx->isLoggedIn) return;
    auto sessions = m_ctx->charges.getSessionHistory(m_ctx->currentUser.userId);
    for (auto& s : sessions) {
        int row = m_historyTable->rowCount();
        m_historyTable->insertRow(row);
        m_historyTable->setItem(row, 0, new QTableWidgetItem(
            QString::number(s.id)));
        m_historyTable->setItem(row, 1, new QTableWidgetItem(
            QString::fromStdString(s.plate)));
        m_historyTable->setItem(row, 2, new QTableWidgetItem(
            fmtTime(s.entryTime)));

        if (s.exitTime == 0) {
            QTableWidgetItem* activeItem = new QTableWidgetItem("Active");
            activeItem->setForeground(QColor("#27AE60"));
            m_historyTable->setItem(row, 3, activeItem);
            m_historyTable->setItem(row, 4, new QTableWidgetItem("—"));
        } else {
            m_historyTable->setItem(row, 3, new QTableWidgetItem(
                fmtTime(s.exitTime)));
            Bill b = m_ctx->charges.getBillForSession(s, m_ctx->currentUser.name);
            QTableWidgetItem* chargeItem = new QTableWidgetItem(
                QString("Rs. %1").arg(b.totalCharge, 0, 'f', 2));
            chargeItem->setForeground(QColor("#2563EB"));
            m_historyTable->setItem(row, 4, chargeItem);
        }
    }
}

void ChargesPage::onStartParking() {
    if (m_vehicleCombo->currentData().isNull()) {
        QMessageBox::warning(this, "No Vehicle",
            "Add a vehicle first in the Vehicle Management page.");
        return;
    }
    QString plate = m_vehicleCombo->currentData().toString();
    if (m_ctx->charges.hasActiveSession(plate.toStdString())) {
        QMessageBox::information(this, "Already Parked",
            plate + " already has an active session.");
        return;
    }
    int sid = m_ctx->charges.startSession(
        m_ctx->currentUser.userId, plate.toStdString());
    if (sid >= 0) {
        m_sessionStatus->setText(QString(
            "✅ Parking started for <b>%1</b>.<br>Session ID: <b>%2</b>")
            .arg(plate).arg(sid));
        emit dataChanged();
        populateHistory();
    } else {
        QMessageBox::warning(this, "Error", "Could not start session.");
    }
}

void ChargesPage::onEndParking() {
    if (m_vehicleCombo->currentData().isNull()) {
        QMessageBox::warning(this, "No Vehicle", "Select a vehicle first.");
        return;
    }
    QString plate = m_vehicleCombo->currentData().toString();
    if (!m_ctx->charges.hasActiveSession(plate.toStdString())) {
        QMessageBox::information(this, "No Active Session",
            plate + " has no active parking session.");
        return;
    }
    Bill b = m_ctx->charges.endSession(
        m_ctx->currentUser.userId,
        plate.toStdString(),
        m_ctx->currentUser.name);
    if (b.sessionId == 0) {
        QMessageBox::warning(this, "Error", "Could not end session.");
        return;
    }
    showBill(b);
    emit dataChanged();
    populateHistory();
}

void ChargesPage::showBill(const Bill& b) {
    m_billCard->show();
    m_billSessionId->setText(QString::number(b.sessionId));
    m_billPlate->setText(QString::fromStdString(b.plate));
    m_billEntry->setText(fmtTime(b.entryTime));
    m_billExit->setText(fmtTime(b.exitTime));
    m_billDuration->setText(QString("%1 min").arg(b.durationMins, 0, 'f', 1));
    m_billRate->setText(QString("Rs. %1/hr").arg(b.ratePerHour, 0, 'f', 0));
    m_billTotal->setText(QString("Rs. %1").arg(b.totalCharge, 0, 'f', 2));
}

void ChargesPage::hideBill() {
    m_billSessionId->setText("—"); m_billPlate->setText("—");
    m_billEntry->setText("—");     m_billExit->setText("—");
    m_billDuration->setText("—");  m_billRate->setText("—");
    m_billTotal->setText("—");
}

void ChargesPage::onSessionSelected(int row, int /*col*/) {
    // Show the bill for the selected historical session
    auto sessions = m_ctx->charges.getSessionHistory(m_ctx->currentUser.userId);
    if (row < 0 || row >= static_cast<int>(sessions.size())) return;
    const ParkingSession& s = sessions[row];
    if (s.exitTime == 0) { hideBill(); return; }
    Bill b = m_ctx->charges.getBillForSession(s, m_ctx->currentUser.name);
    showBill(b);
}
