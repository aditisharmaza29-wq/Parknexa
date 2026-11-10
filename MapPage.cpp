// ============================================================================
//  MapPage.cpp  –  Mall graph visualisation with Dijkstra route highlight
// ============================================================================
#include "MapPage.h"
#include "../services/AppContext.h"
#include "../core/ParkingTypes.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QGraphicsEllipseItem>
#include <QGraphicsLineItem>
#include <QGraphicsTextItem>
#include <QPen>
#include <QBrush>
#include <QFont>
#include <cmath>

MapPage::MapPage(AppContext* ctx, QWidget* parent)
    : QWidget(parent), m_ctx(ctx)
{
    buildUi();
    buildNodePositions();
    refresh();
}

void MapPage::buildUi() {
    QVBoxLayout* root = new QVBoxLayout(this);
    root->setContentsMargins(24, 24, 24, 24);
    root->setSpacing(16);

    QLabel* title = new QLabel("Mall Layout & Route Navigation");
    title->setObjectName("pageTitle");
    QLabel* sub = new QLabel(
        "Graph visualisation of the mall. Highlighted path = Dijkstra shortest route.");
    sub->setObjectName("pageSubtitle");
    root->addWidget(title);
    root->addWidget(sub);

    // Controls
    QHBoxLayout* ctrlRow = new QHBoxLayout;
    QLabel* selLbl = new QLabel("Show route to:");
    selLbl->setObjectName("formLabel");
    m_slotCombo = new QComboBox;
    m_slotCombo->addItem("— Select a slot —", -1);
    for (int i = 1; i <= 9; i++)
        m_slotCombo->addItem(QString("Slot S%1").arg(i), i);
    connect(m_slotCombo,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MapPage::onSlotSelected);

    m_routeLabel = new QLabel("No route selected.");
    m_routeLabel->setObjectName("pageSubtitle");
    m_routeLabel->setWordWrap(true);

    m_distLabel = new QLabel;
    m_distLabel->setObjectName("pageSubtitle");

    ctrlRow->addWidget(selLbl);
    ctrlRow->addWidget(m_slotCombo);
    ctrlRow->addStretch();
    root->addLayout(ctrlRow);
    root->addWidget(m_routeLabel);
    root->addWidget(m_distLabel);

    // Graphics view
    m_scene = new QGraphicsScene(this);
    m_view  = new QGraphicsView(m_scene, this);
    m_view->setObjectName("mapView");
    m_view->setRenderHint(QPainter::Antialiasing);
    m_view->setDragMode(QGraphicsView::ScrollHandDrag);
    m_view->setMinimumHeight(380);
    root->addWidget(m_view, 1);

    // Legend
    QHBoxLayout* legendRow = new QHBoxLayout;
    auto addLeg = [&](const QColor& c, const QString& text) {
        QLabel* dot = new QLabel("●");
        dot->setStyleSheet(QString("color: %1; font-size: 16px;").arg(c.name()));
        QLabel* lbl = new QLabel(text);
        lbl->setStyleSheet("color: #4B5563; font-size: 12px; margin-right: 14px;");
        legendRow->addWidget(dot); legendRow->addWidget(lbl);
    };
    addLeg(QColor("#27AE60"), "Available slot");
    addLeg(QColor("#F39C12"), "Reserved slot");
    addLeg(QColor("#E74C3C"), "Occupied slot");
    addLeg(QColor("#60A5FA"), "Dijkstra route");
    addLeg(QColor("#FFFFFF"), "Junction / Entrance");
    legendRow->addStretch();
    root->addLayout(legendRow);
}

// ---------------------------------------------------------------------------
//  Node positions (manually laid out to resemble the mall graph)
//  13 nodes: 0=Entrance, 1=JunctionA, 2=JunctionB, 3=JunctionC,
//            4-12 = Slots S1..S9
// ---------------------------------------------------------------------------
void MapPage::buildNodePositions() {
    m_nodePos.resize(13);
    // Entrance at top-centre
    m_nodePos[0]  = {400, 40};
    // Junctions
    m_nodePos[1]  = {400, 160};  // Junction A (Wing A)
    m_nodePos[2]  = {200, 300};  // Junction B (Wing B)
    m_nodePos[3]  = {600, 300};  // Junction C (Wing C)
    // Wing A slots (around Junction A)
    m_nodePos[4]  = {260, 180};  // S1 Accessible
    m_nodePos[5]  = {280, 240};  // S2 Regular
    m_nodePos[6]  = {360, 270};  // S3 Compact
    // Wing B slots (around Junction B)
    m_nodePos[7]  = {80,  300};  // S4 EV
    m_nodePos[8]  = {120, 380};  // S5 Regular
    m_nodePos[9]  = {200, 430};  // S6 Regular
    // Wing C slots (around Junction C)
    m_nodePos[10] = {620, 220};  // S7 Compact
    m_nodePos[11] = {700, 320};  // S8 Regular
    m_nodePos[12] = {660, 430};  // S9 EV
}

// ---------------------------------------------------------------------------
//  getRouteNodes  --  reconstruct Dijkstra path node indices
// ---------------------------------------------------------------------------
QVector<int> MapPage::getRouteNodes(int slotId) const {
    RoutePath names = m_ctx->parking.getRoute(slotId);
    // Map names to node indices (by position in nodeName list)
    auto allNames = m_ctx->parking.getNodeNames();
    QVector<int> indices;
    for (auto& name : names) {
        for (int i = 0; i < static_cast<int>(allNames.size()); i++) {
            if (allNames[i] == name) { indices.append(i); break; }
        }
    }
    return indices;
}

// ---------------------------------------------------------------------------
//  drawGraph  --  render everything on the QGraphicsScene
// ---------------------------------------------------------------------------
void MapPage::drawGraph(int highlightSlotId, const QVector<int>& routeNodes) {
    m_scene->clear();
    m_scene->setBackgroundBrush(QBrush(QColor("#1A2744")));

    auto allSlots = m_ctx->parking.getSlots();
    auto edges = m_ctx->parking.getEdges();
    auto names = m_ctx->parking.getNodeNames();

    // Build a set of route edges for fast lookup
    QSet<QPair<int,int>> routeEdges;
    for (int i = 0; i + 1 < routeNodes.size(); i++) {
        int a = routeNodes[i], b = routeNodes[i+1];
        routeEdges.insert({std::min(a,b), std::max(a,b)});
    }

    // Draw edges
    for (auto& e : edges) {
        if (e.from >= static_cast<int>(m_nodePos.size()) ||
            e.to   >= static_cast<int>(m_nodePos.size())) continue;
        QPointF p1 = m_nodePos[e.from];
        QPointF p2 = m_nodePos[e.to];
        bool isRoute = routeEdges.contains(
            {std::min(e.from,e.to), std::max(e.from,e.to)});

        QPen pen(isRoute ? QColor("#60A5FA") : QColor("#2D3E5A"),
                 isRoute ? 4 : 2, Qt::SolidLine, Qt::RoundCap);
        m_scene->addLine(QLineF(p1, p2), pen);

        // Edge weight label
        QPointF mid = (p1 + p2) / 2.0;
        QGraphicsTextItem* wt = m_scene->addText(QString::number(e.weight));
        wt->setDefaultTextColor(QColor("#8FA3C1"));
        QFont wf; wf.setPointSize(8); wt->setFont(wf);
        wt->setPos(mid.x() - 8, mid.y() - 8);
    }

    // Draw nodes
    const double R = 20.0;
    for (int i = 0; i < static_cast<int>(m_nodePos.size()); i++) {
        QPointF p = m_nodePos[i];
        bool isEntrance  = (i == 0);
        bool isJunction  = (i >= 1 && i <= 3);
        bool isSlotNode  = (i >= 4);
        int  slotId      = isSlotNode ? (i - 3) : -1;  // node 4 = slot S1
        bool inRoute     = routeNodes.contains(i);

        // Determine slot status
        QColor nodeColor = QColor("#4B5563");  // default gray
        if (isEntrance) nodeColor = QColor("#FBBF24");
        else if (isJunction) nodeColor = QColor("#FFFFFF");
        else if (isSlotNode) {
            // Find corresponding slot
            bool found = false;
            for (auto& s : allSlots) {
                if (s.id == slotId) {
                    if (!s.occupied)      nodeColor = QColor("#27AE60");
                    else if (s.resId!=0)  nodeColor = QColor("#F39C12");
                    else                  nodeColor = QColor("#E74C3C");
                    found = true;
                    break;
                }
            }
            if (!found) nodeColor = QColor("#6B7280");
        }

        // Highlight route nodes
        QPen border(inRoute ? QColor("#60A5FA") : Qt::NoPen, inRoute ? 3 : 0);
        m_scene->addEllipse(p.x()-R, p.y()-R, 2*R, 2*R, border, QBrush(nodeColor));

        // Node label
        QString label = QString::fromStdString(names[i]);
        // Short label for slots
        if (isSlotNode) label = QString("S%1").arg(slotId);
        QGraphicsTextItem* txt = m_scene->addText(label);
        txt->setDefaultTextColor(
            (isJunction||isEntrance) ? QColor("#1A2744") : QColor("#FFFFFF"));
        QFont f; f.setPointSize(8); f.setBold(true); txt->setFont(f);
        txt->setPos(p.x() - txt->boundingRect().width()/2, p.y() - 8);
    }

    m_view->fitInView(m_scene->itemsBoundingRect(), Qt::KeepAspectRatio);
}

void MapPage::refresh() {
    drawGraph();
}

void MapPage::onSlotSelected(int index) {
    int slotId = m_slotCombo->itemData(index).toInt();
    if (slotId < 0) {
        m_routeLabel->setText("No route selected.");
        m_distLabel->clear();
        drawGraph();
        return;
    }

    auto routeNodes = getRouteNodes(slotId);
    RoutePath path  = m_ctx->parking.getRoute(slotId);
    Slot s          = m_ctx->parking.getSlotById(slotId);

    QString routeStr = "Route: ";
    for (int i = 0; i < static_cast<int>(path.size()); i++) {
        routeStr += QString::fromStdString(path[i]);
        if (i + 1 < static_cast<int>(path.size())) routeStr += " → ";
    }
    m_routeLabel->setText(routeStr);
    m_distLabel->setText(QString("Total distance from entrance: <b>%1 m</b>").arg(s.dist));

    drawGraph(slotId, routeNodes);
}
