// ============================================================================
//  MapPage.h  –  Mall graph visualisation using QGraphicsView
//               Shows the Dijkstra route for the current reservation
// ============================================================================
#pragma once
#include <QWidget>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QLabel>
#include <QComboBox>
#include <QVector>
#include <QPointF>

class AppContext;

class MapPage : public QWidget {
    Q_OBJECT
public:
    explicit MapPage(AppContext* ctx, QWidget* parent = nullptr);
    void refresh();

private slots:
    void onSlotSelected(int index);

private:
    AppContext*      m_ctx;
    QGraphicsView*   m_view;
    QGraphicsScene*  m_scene;
    QComboBox*       m_slotCombo;
    QLabel*          m_routeLabel;
    QLabel*          m_distLabel;

    // Fixed node positions on the canvas
    QVector<QPointF> m_nodePos;

    void buildUi();
    void buildNodePositions();
    void drawGraph(int highlightSlotId = -1, const QVector<int>& routeNodes = {});
    QVector<int> getRouteNodes(int slotId) const;
};
