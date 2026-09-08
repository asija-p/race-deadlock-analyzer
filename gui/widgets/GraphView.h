#ifndef GRAPHVIEW_H
#define GRAPHVIEW_H

#include <QGraphicsView>
#include <QString>
#include <QPair>
#include <QVector>
#include <QSet>

class QLabel;

class GraphView : public QGraphicsView {
public:
    explicit GraphView(QWidget *parent = nullptr);

    void SetGraph(const QVector<QPair<QString, QString>> &Edges,
                  const QSet<QString> &CycleEdges);

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    QGraphicsScene *Scene;
    QLabel *LegendLabel;

    void RepositionLegend();
};

#endif