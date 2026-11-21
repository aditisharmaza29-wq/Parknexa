// ============================================================================
//  MainWindow.h  –  Application shell with sidebar + stacked pages
// ============================================================================
#pragma once
#include <QMainWindow>
#include <QStackedWidget>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QVector>

class AppContext;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(AppContext* ctx, QWidget* parent = nullptr);

    // Called when a page wants to refresh the whole dashboard
    void refreshAll();

signals:
    void logoutRequested();

private slots:
    void onNavClicked(int pageIndex);
    void onLogout();

private:
    AppContext*     m_ctx;
    QStackedWidget* m_stack;
    QVBoxLayout*    m_navLayout;
    QVector<QPushButton*> m_navBtns;

    // Sidebar user info labels
    QLabel* m_userNameLabel;
    QLabel* m_userVehicleLabel;

    void buildUi();
    QWidget* buildSidebar();
    QWidget* buildContent();
    void setActivePage(int index);
    void updateUserInfo();
};
