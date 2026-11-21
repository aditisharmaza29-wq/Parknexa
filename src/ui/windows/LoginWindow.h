// ============================================================================
//  LoginWindow.h  –  Login and registration screen
// ============================================================================
#pragma once
#include <QWidget>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QTabWidget>
#include <QComboBox>

class AppContext;

class LoginWindow : public QWidget {
    Q_OBJECT
public:
    explicit LoginWindow(AppContext* ctx, QWidget* parent = nullptr);

signals:
    void loginSuccess();   // emitted after successful authentication

private slots:
    void onLoginClicked();
    void onRegisterClicked();

private:
    AppContext* m_ctx;

    // Login tab fields
    QLineEdit*  m_loginUser;
    QLineEdit*  m_loginPass;
    QLabel*     m_loginError;

    // Register tab fields
    QLineEdit*  m_regName;
    QLineEdit*  m_regUser;
    QLineEdit*  m_regPass;
    QLineEdit*  m_regPassConfirm;
    QLineEdit*  m_regPlate;
    QComboBox*  m_regVehicle;
    QLabel*     m_regError;

    void buildUi();
    QWidget* makeLoginTab();
    QWidget* makeRegisterTab();
};
