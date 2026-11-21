// ============================================================================
//  MainWindow.cpp  –  Application shell with sidebar + stacked pages
// ============================================================================
#include "MainWindow.h"
#include "DashboardPage.h"
#include "ParkingPage.h"
#include "AllocationPage.h"
#include "VehiclePage.h"
#include "ChargesPage.h"
#include "HistoryPage.h"
#include "MapPage.h"
#include "../services/AppContext.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QFrame>
#include <QSpacerItem>
#include <QMessageBox>

MainWindow::MainWindow(AppContext* ctx, QWidget* parent)
    : QMainWindow(parent), m_ctx(ctx)
{
    setWindowTitle("ParkNexa – Adaptive Parking Management");
    setMinimumSize(1100, 700);
    resize(1280, 760);
    buildUi();
    setActivePage(0);
    updateUserInfo();
}

void MainWindow::refreshAll() {
    // Refresh the currently visible page
    if (auto* p = qobject_cast<DashboardPage*>(m_stack->currentWidget()))  p->refresh();
    if (auto* p = qobject_cast<ParkingPage*>(m_stack->currentWidget()))    p->refresh();
    if (auto* p = qobject_cast<AllocationPage*>(m_stack->currentWidget())) p->refresh();
    if (auto* p = qobject_cast<VehiclePage*>(m_stack->currentWidget()))    p->refresh();
    if (auto* p = qobject_cast<ChargesPage*>(m_stack->currentWidget()))    p->refresh();
    if (auto* p = qobject_cast<HistoryPage*>(m_stack->currentWidget()))    p->refresh();
    if (auto* p = qobject_cast<MapPage*>(m_stack->currentWidget()))        p->refresh();
}

// ---------------------------------------------------------------------------
//  buildUi
// ---------------------------------------------------------------------------
void MainWindow::buildUi() {
    QWidget* central = new QWidget;
    setCentralWidget(central);

    QHBoxLayout* hlay = new QHBoxLayout(central);
    hlay->setContentsMargins(0, 0, 0, 0);
    hlay->setSpacing(0);

    hlay->addWidget(buildSidebar());

    // Thin separator
    QFrame* sep = new QFrame;
    sep->setFrameShape(QFrame::VLine);
    sep->setObjectName("separator");
    hlay->addWidget(sep);

    hlay->addWidget(buildContent(), 1);
}

// ---------------------------------------------------------------------------
//  buildSidebar  --  dark navy navigation sidebar
// ---------------------------------------------------------------------------
QWidget* MainWindow::buildSidebar() {
    QWidget* sidebar = new QWidget;
    sidebar->setObjectName("sidebar");
    sidebar->setFixedWidth(220);

    QVBoxLayout* vlay = new QVBoxLayout(sidebar);
    vlay->setContentsMargins(0, 0, 0, 0);
    vlay->setSpacing(0);

    // App title
    QLabel* logoLbl = new QLabel("🅿 ParkNexa");
    logoLbl->setObjectName("appTitle");

    QLabel* subLbl = new QLabel("Parking Management");
    subLbl->setObjectName("appSubtitle");

    vlay->addWidget(logoLbl);
    vlay->addWidget(subLbl);

    // Separator line
    QFrame* div = new QFrame; div->setObjectName("separator");
    div->setFrameShape(QFrame::HLine); div->setMaximumHeight(1);
    vlay->addWidget(div);
    vlay->addSpacing(8);

    // Nav buttons
    struct NavItem { QString icon; QString label; };
    QVector<NavItem> items = {
        {"📊", "Dashboard"},
        {"🅿",  "Parking Slots"},
        {"🎯", "Allocate Slot"},
        {"🚗", "Vehicles"},
        {"💰", "Charges & Billing"},
        {"📋", "Reservation History"},
        {"🗺",  "Mall Map"},
    };

    m_navLayout = new QVBoxLayout;
    m_navLayout->setSpacing(2);
    m_navLayout->setContentsMargins(0, 0, 0, 0);

    for (int i = 0; i < items.size(); i++) {
        QPushButton* btn = new QPushButton(
            items[i].icon + "  " + items[i].label);
        btn->setObjectName("navButton");
        btn->setCheckable(false);
        btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        btn->setFixedHeight(46);
        btn->setFlat(true);
        btn->setCursor(Qt::PointingHandCursor);
        connect(btn, &QPushButton::clicked, this, [this, i](){ onNavClicked(i); });
        m_navBtns.append(btn);
        m_navLayout->addWidget(btn);
    }

    vlay->addLayout(m_navLayout);
    vlay->addStretch();

    // Separator
    QFrame* div2 = new QFrame; div2->setObjectName("separator");
    div2->setFrameShape(QFrame::HLine); div2->setMaximumHeight(1);
    vlay->addWidget(div2);

    // User info panel
    QWidget* userPanel = new QWidget;
    userPanel->setObjectName("userPanel");
    QVBoxLayout* upLay = new QVBoxLayout(userPanel);
    upLay->setContentsMargins(8, 8, 8, 8);
    upLay->setSpacing(2);

    m_userNameLabel    = new QLabel;
    m_userNameLabel->setObjectName("userNameLabel");
    m_userVehicleLabel = new QLabel;
    m_userVehicleLabel->setObjectName("userRoleLabel");

    upLay->addWidget(m_userNameLabel);
    upLay->addWidget(m_userVehicleLabel);
    vlay->addWidget(userPanel);

    // Logout button
    QPushButton* logoutBtn = new QPushButton("Logout");
    logoutBtn->setObjectName("logoutButton");
    connect(logoutBtn, &QPushButton::clicked, this, &MainWindow::onLogout);
    vlay->addWidget(logoutBtn);
    vlay->addSpacing(8);

    return sidebar;
}

// ---------------------------------------------------------------------------
//  buildContent  --  stacked pages (one per nav item)
// ---------------------------------------------------------------------------
QWidget* MainWindow::buildContent() {
    QWidget* content = new QWidget;
    content->setObjectName("contentArea");

    QVBoxLayout* lay = new QVBoxLayout(content);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(0);

    m_stack = new QStackedWidget;

    // Create pages in exact same order as nav buttons
    auto* dash     = new DashboardPage(m_ctx);
    auto* parking  = new ParkingPage(m_ctx);
    auto* alloc    = new AllocationPage(m_ctx);
    auto* vehicles = new VehiclePage(m_ctx);
    auto* charges  = new ChargesPage(m_ctx);
    auto* history  = new HistoryPage(m_ctx);
    auto* mapPage  = new MapPage(m_ctx);

    m_stack->addWidget(dash);      // 0
    m_stack->addWidget(parking);   // 1
    m_stack->addWidget(alloc);     // 2
    m_stack->addWidget(vehicles);  // 3
    m_stack->addWidget(charges);   // 4
    m_stack->addWidget(history);   // 5
    m_stack->addWidget(mapPage);   // 6

    // Connect data-changed signals to refresh dashboard
    connect(alloc,   &AllocationPage::dataChanged, dash, &DashboardPage::refresh);
    connect(alloc,   &AllocationPage::dataChanged, parking, &ParkingPage::refresh);
    connect(alloc,   &AllocationPage::dataChanged, mapPage, &MapPage::refresh);
    connect(parking, &ParkingPage::dataChanged,    dash, &DashboardPage::refresh);
    connect(parking, &ParkingPage::dataChanged,    alloc, &AllocationPage::refresh);
    connect(charges, &ChargesPage::dataChanged,    dash, &DashboardPage::refresh);

    lay->addWidget(m_stack);
    return content;
}

// ---------------------------------------------------------------------------
//  setActivePage  --  switch stack and update sidebar button styles
// ---------------------------------------------------------------------------
void MainWindow::setActivePage(int index) {
    m_stack->setCurrentIndex(index);
    for (int i = 0; i < m_navBtns.size(); i++) {
        // Use dynamic property to control QSS active style
        m_navBtns[i]->setProperty("active", i == index);
        m_navBtns[i]->style()->unpolish(m_navBtns[i]);
        m_navBtns[i]->style()->polish(m_navBtns[i]);
    }

    // Refresh the newly shown page
    if (index == 0) qobject_cast<DashboardPage*>(m_stack->widget(0))->refresh();
    if (index == 1) qobject_cast<ParkingPage*>(m_stack->widget(1))->refresh();
    if (index == 2) qobject_cast<AllocationPage*>(m_stack->widget(2))->refresh();
    if (index == 3) qobject_cast<VehiclePage*>(m_stack->widget(3))->refresh();
    if (index == 4) qobject_cast<ChargesPage*>(m_stack->widget(4))->refresh();
    if (index == 5) qobject_cast<HistoryPage*>(m_stack->widget(5))->refresh();
    if (index == 6) qobject_cast<MapPage*>(m_stack->widget(6))->refresh();
}

void MainWindow::updateUserInfo() {
    if (m_ctx->isLoggedIn) {
        m_userNameLabel->setText(
            QString::fromStdString(m_ctx->currentUser.name));
        QString veh;
        switch (m_ctx->currentUser.vehicle) {
            case 1: veh = "Bike/Compact"; break;
            case 2: veh = "Car";          break;
            case 3: veh = "EV";           break;
            case 4: veh = "Accessible";   break;
        }
        m_userVehicleLabel->setText(
            "@" + QString::fromStdString(m_ctx->currentUser.userId)
            + " · " + veh);
    }
}

void MainWindow::onNavClicked(int pageIndex) {
    setActivePage(pageIndex);
}

void MainWindow::onLogout() {
    auto ans = QMessageBox::question(this, "Logout",
        "Are you sure you want to logout?",
        QMessageBox::Yes | QMessageBox::No);
    if (ans == QMessageBox::Yes) {
        m_ctx->isLoggedIn = false;
        emit logoutRequested();
    }
}
