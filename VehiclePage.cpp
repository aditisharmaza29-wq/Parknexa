// ============================================================================
//  VehiclePage.cpp
// ============================================================================
#include "VehiclePage.h"
#include "../services/AppContext.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QMessageBox>
#include <QPushButton>
#include <QFormLayout>

VehiclePage::VehiclePage(AppContext* ctx, QWidget* parent)
    : QWidget(parent), m_ctx(ctx)
{
    buildUi();
    refresh();
}

void VehiclePage::buildUi() {
    QVBoxLayout* root = new QVBoxLayout(this);
    root->setContentsMargins(24, 24, 24, 24);
    root->setSpacing(16);

    QLabel* title = new QLabel("Vehicle Management");
    title->setObjectName("pageTitle");
    QLabel* sub = new QLabel("Add, remove and search registered vehicles");
    sub->setObjectName("pageSubtitle");
    root->addWidget(title);
    root->addWidget(sub);

    QHBoxLayout* mainRow = new QHBoxLayout;
    mainRow->setSpacing(20);

    // ── Left: Add vehicle form ────────────────────────────────────────────────
    QGroupBox* addBox = new QGroupBox("Add Vehicle");
    addBox->setMaximumWidth(320);
    QFormLayout* form = new QFormLayout(addBox);
    form->setSpacing(12);
    form->setContentsMargins(16, 20, 16, 16);

    m_plateEdit = new QLineEdit;
    m_plateEdit->setPlaceholderText("e.g. DL01AB1234");

    m_typeCombo = new QComboBox;
    m_typeCombo->addItem("Bike/Scooter");
    m_typeCombo->addItem("Car");
    m_typeCombo->addItem("Electric Vehicle");
    m_typeCombo->addItem("Accessible Vehicle");

    m_addError = new QLabel;
    m_addError->setObjectName("errorLabel");
    m_addError->setWordWrap(true);
    m_addError->hide();

    QPushButton* addBtn = new QPushButton("Add Vehicle");
    addBtn->setObjectName("primaryBtn");
    addBtn->setFixedHeight(40);
    connect(addBtn, &QPushButton::clicked, this, &VehiclePage::onAddVehicle);

    form->addRow("Vehicle Plate:", m_plateEdit);
    form->addRow("Type:",          m_typeCombo);
    form->addRow("",               m_addError);
    form->addRow("",               addBtn);

    // Current user info
    QGroupBox* infoBox = new QGroupBox("Your Account");
    QFormLayout* infoForm = new QFormLayout(infoBox);
    if (m_ctx->isLoggedIn) {
        infoForm->addRow("User:", new QLabel(
            QString::fromStdString(m_ctx->currentUser.userId)));
        infoForm->addRow("Name:", new QLabel(
            QString::fromStdString(m_ctx->currentUser.name)));
        infoForm->addRow("Registered Plate:", new QLabel(
            QString::fromStdString(m_ctx->currentUser.plate)));
    }

    QVBoxLayout* leftVlay = new QVBoxLayout;
    leftVlay->addWidget(infoBox);
    leftVlay->addWidget(addBox);
    leftVlay->addStretch();

    // ── Right: Vehicle list ───────────────────────────────────────────────────
    QVBoxLayout* rightVlay = new QVBoxLayout;

    QHBoxLayout* searchRow = new QHBoxLayout;
    QLabel* searchLbl = new QLabel("Search:");
    m_searchEdit = new QLineEdit;
    m_searchEdit->setPlaceholderText("Filter by plate or type…");
    QPushButton* searchBtn = new QPushButton("Search");
    searchBtn->setObjectName("secondaryBtn");
    connect(searchBtn, &QPushButton::clicked, this, &VehiclePage::onSearch);
    connect(m_searchEdit, &QLineEdit::textChanged, this, &VehiclePage::onSearch);
    searchRow->addWidget(searchLbl);
    searchRow->addWidget(m_searchEdit);
    searchRow->addWidget(searchBtn);

    m_vehicleTable = new QTableWidget(0, 3);
    m_vehicleTable->setHorizontalHeaderLabels({"Plate", "Type", "Action"});
    m_vehicleTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_vehicleTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_vehicleTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_vehicleTable->setAlternatingRowColors(true);
    m_vehicleTable->verticalHeader()->hide();

    rightVlay->addLayout(searchRow);
    rightVlay->addWidget(m_vehicleTable);

    mainRow->addLayout(leftVlay, 1);
    mainRow->addLayout(rightVlay, 2);
    root->addLayout(mainRow, 1);
}

void VehiclePage::refresh() {
    populateTable();
}

void VehiclePage::populateTable(const QString& filter) {
    auto vehicles = m_ctx->charges.getVehiclesForUser(m_ctx->currentUser.userId);
    m_vehicleTable->setRowCount(0);
    for (auto& v : vehicles) {
        QString plate = QString::fromStdString(v.plate);
        QString type  = QString::fromStdString(v.type);
        if (!filter.isEmpty() &&
            !plate.contains(filter, Qt::CaseInsensitive) &&
            !type.contains(filter, Qt::CaseInsensitive))
            continue;

        int row = m_vehicleTable->rowCount();
        m_vehicleTable->insertRow(row);
        m_vehicleTable->setItem(row, 0, new QTableWidgetItem(plate));
        m_vehicleTable->setItem(row, 1, new QTableWidgetItem(type));

        // Remove button
        QPushButton* rmBtn = new QPushButton("Remove");
        rmBtn->setObjectName("dangerBtn");
        rmBtn->setFixedHeight(28);
        QString pCopy = plate;
        connect(rmBtn, &QPushButton::clicked, this, [this, pCopy](){
            auto ans = QMessageBox::question(this, "Remove Vehicle",
                "Remove vehicle " + pCopy + "?",
                QMessageBox::Yes | QMessageBox::No);
            if (ans == QMessageBox::Yes) {
                std::string err = m_ctx->charges.removeVehicle(
                    m_ctx->currentUser.userId, pCopy.toStdString());
                if (err.empty()) {
                    QMessageBox::information(this, "Removed",
                        pCopy + " removed.");
                    populateTable();
                } else {
                    QMessageBox::warning(this, "Error",
                        QString::fromStdString(err));
                }
            }
        });
        m_vehicleTable->setCellWidget(row, 2, rmBtn);
    }
}

void VehiclePage::onAddVehicle() {
    m_addError->hide();
    QString plate = m_plateEdit->text().trimmed().toUpper();
    QString type  = m_typeCombo->currentText();

    std::string err = m_ctx->charges.addVehicle(
        m_ctx->currentUser.userId, plate.toStdString(), type.toStdString());
    if (!err.empty()) {
        m_addError->setText(QString::fromStdString(err));
        m_addError->show(); return;
    }
    m_plateEdit->clear();
    m_addError->setObjectName("successMsg");
    m_addError->setText("✓ Vehicle " + plate + " added.");
    m_addError->show();
    populateTable();
}

void VehiclePage::onRemoveVehicle() {
    // handled inline in the table's Remove buttons
}

void VehiclePage::onSearch() {
    populateTable(m_searchEdit->text().trimmed());
}
