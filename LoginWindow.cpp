// ============================================================================
//  LoginWindow.cpp  –  Login and registration screen implementation
// ============================================================================
#include "LoginWindow.h"
#include "../services/AppContext.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QTabWidget>
#include <QFrame>
#include <QSpacerItem>

LoginWindow::LoginWindow(AppContext* ctx, QWidget* parent)
    : QWidget(parent), m_ctx(ctx)
{
    buildUi();
}

// ---------------------------------------------------------------------------
//  buildUi  --  constructs the entire login screen
// ---------------------------------------------------------------------------
void LoginWindow::buildUi() {
    // Full-window dark background
    setObjectName("loginBg");
    setStyleSheet("#loginBg { background-color: #1A2744; }");

    QVBoxLayout* outer = new QVBoxLayout(this);
    outer->setAlignment(Qt::AlignCenter);

    // White card container
    QWidget* card = new QWidget;
    card->setObjectName("loginContainer");
    card->setFixedWidth(430);

    QVBoxLayout* vlay = new QVBoxLayout(card);
    vlay->setSpacing(16);
    vlay->setContentsMargins(40, 40, 40, 40);

    // Logo + title
    QLabel* logo = new QLabel("🅿 ParkNexa");
    logo->setObjectName("loginLogo");
    logo->setAlignment(Qt::AlignCenter);

    QLabel* sub = new QLabel("Adaptive Shopping Mall Parking System");
    sub->setObjectName("loginSubtitle");
    sub->setAlignment(Qt::AlignCenter);

    // Tab widget: Login | Register
    QTabWidget* tabs = new QTabWidget;
    tabs->setDocumentMode(true);
    tabs->addTab(makeLoginTab(),    "Login");
    tabs->addTab(makeRegisterTab(), "Register");

    vlay->addWidget(logo);
    vlay->addWidget(sub);
    vlay->addSpacing(8);
    vlay->addWidget(tabs);

    outer->addWidget(card);
}

// ---------------------------------------------------------------------------
//  Login tab
// ---------------------------------------------------------------------------
QWidget* LoginWindow::makeLoginTab() {
    QWidget* w = new QWidget;
    QVBoxLayout* lay = new QVBoxLayout(w);
    lay->setSpacing(14);
    lay->setContentsMargins(0, 16, 0, 8);

    // User ID
    QLabel* lUser = new QLabel("User ID");
    lUser->setObjectName("formLabel");
    m_loginUser = new QLineEdit;
    m_loginUser->setPlaceholderText("Enter your user ID");

    // Password
    QLabel* lPass = new QLabel("Password");
    lPass->setObjectName("formLabel");
    m_loginPass = new QLineEdit;
    m_loginPass->setEchoMode(QLineEdit::Password);
    m_loginPass->setPlaceholderText("Enter your password");

    // Error message
    m_loginError = new QLabel;
    m_loginError->setObjectName("errorLabel");
    m_loginError->setWordWrap(true);
    m_loginError->hide();

    // Login button
    QPushButton* btn = new QPushButton("Login");
    btn->setObjectName("primaryBtn");
    btn->setFixedHeight(44);
    connect(btn, &QPushButton::clicked, this, &LoginWindow::onLoginClicked);
    // Also allow Enter key
    connect(m_loginPass, &QLineEdit::returnPressed, this, &LoginWindow::onLoginClicked);

    lay->addWidget(lUser);
    lay->addWidget(m_loginUser);
    lay->addWidget(lPass);
    lay->addWidget(m_loginPass);
    lay->addWidget(m_loginError);
    lay->addSpacing(4);
    lay->addWidget(btn);

    return w;
}

// ---------------------------------------------------------------------------
//  Register tab
// ---------------------------------------------------------------------------
QWidget* LoginWindow::makeRegisterTab() {
    QWidget* w = new QWidget;
    QVBoxLayout* lay = new QVBoxLayout(w);
    lay->setSpacing(10);
    lay->setContentsMargins(0, 16, 0, 8);

    auto addField = [&](const QString& label, QLineEdit*& field,
                        const QString& placeholder, bool isPass = false) {
        QLabel* lbl = new QLabel(label);
        lbl->setObjectName("formLabel");
        field = new QLineEdit;
        field->setPlaceholderText(placeholder);
        if (isPass) field->setEchoMode(QLineEdit::Password);
        lay->addWidget(lbl);
        lay->addWidget(field);
    };

    addField("Full Name",         m_regName,        "e.g. Priya Sharma");
    addField("User ID",           m_regUser,        "4-15 chars, letters/digits/_");
    addField("Password",          m_regPass,        "Min 6 chars, 1 letter + 1 digit", true);
    addField("Confirm Password",  m_regPassConfirm, "Repeat password", true);
    addField("Vehicle Plate",     m_regPlate,       "e.g. UK07CA5678");

    // Vehicle type combo
    QLabel* lVeh = new QLabel("Vehicle Type");
    lVeh->setObjectName("formLabel");
    m_regVehicle = new QComboBox;
    m_regVehicle->addItem("Bike / Compact",          1);
    m_regVehicle->addItem("Car",                     2);
    m_regVehicle->addItem("Electric Vehicle (EV)",   3);
    m_regVehicle->addItem("Accessible (permit)",     4);

    // Error label
    m_regError = new QLabel;
    m_regError->setObjectName("errorLabel");
    m_regError->setWordWrap(true);
    m_regError->hide();

    // Register button
    QPushButton* btn = new QPushButton("Create Account");
    btn->setObjectName("primaryBtn");
    btn->setFixedHeight(44);
    connect(btn, &QPushButton::clicked, this, &LoginWindow::onRegisterClicked);

    lay->addWidget(lVeh);
    lay->addWidget(m_regVehicle);
    lay->addWidget(m_regError);
    lay->addSpacing(4);
    lay->addWidget(btn);

    return w;
}

// ---------------------------------------------------------------------------
//  Slot: Login button clicked
// ---------------------------------------------------------------------------
void LoginWindow::onLoginClicked() {
    m_loginError->hide();
    QString userId = m_loginUser->text().trimmed();
    QString pass   = m_loginPass->text();

    if (userId.isEmpty() || pass.isEmpty()) {
        m_loginError->setText("Please enter your user ID and password.");
        m_loginError->show();
        return;
    }

    std::string err;
    Account* acc = m_ctx->auth.login(userId.toStdString(), pass.toStdString(), err);
    if (!acc) {
        m_loginError->setText(QString::fromStdString(err));
        m_loginError->show();
        m_loginPass->clear();
        return;
    }

    m_ctx->currentUser = *acc;
    m_ctx->isLoggedIn  = true;
    m_loginUser->clear();
    m_loginPass->clear();
    emit loginSuccess();
}

// ---------------------------------------------------------------------------
//  Slot: Register button clicked
// ---------------------------------------------------------------------------
void LoginWindow::onRegisterClicked() {
    m_regError->hide();

    QString name    = m_regName->text().trimmed();
    QString userId  = m_regUser->text().trimmed();
    QString pass    = m_regPass->text();
    QString confirm = m_regPassConfirm->text();
    QString plate   = m_regPlate->text().trimmed().toUpper();
    int     vType   = m_regVehicle->currentData().toInt();

    if (name.isEmpty() || userId.isEmpty() || pass.isEmpty() || plate.isEmpty()) {
        m_regError->setText("All fields are required.");
        m_regError->show(); return;
    }
    if (pass != confirm) {
        m_regError->setText("Passwords do not match.");
        m_regError->show(); return;
    }

    std::string err = m_ctx->auth.registerAccount(
        userId.toStdString(), pass.toStdString(),
        name.toStdString(), plate.toStdString(), vType);

    if (!err.empty()) {
        m_regError->setText(QString::fromStdString(err));
        m_regError->show(); return;
    }

    // Success — clear fields and show a nice message
    m_regName->clear(); m_regUser->clear(); m_regPass->clear();
    m_regPassConfirm->clear(); m_regPlate->clear();
    m_regError->setObjectName("successMsg");
    m_regError->setText("✓ Account created! You can now log in.");
    m_regError->show();
}
