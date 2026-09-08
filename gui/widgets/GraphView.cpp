#include "GraphView.h"
#include <QGraphicsScene>
#include <QGraphicsEllipseItem>
#include <QGraphicsTextItem>
#include <QGraphicsPathItem>
#include <QGraphicsPolygonItem>
#include <QPainterPath>
#include <QPen>
#include <QBrush>
#include <QLabel>
#include <QResizeEvent>
#include <cmath>

GraphView::GraphView(QWidget *parent) : QGraphicsView(parent) {
    Scene = new QGraphicsScene(this);
    setScene(Scene);
    setRenderHint(QPainter::Antialiasing);

    LegendLabel = new QLabel(this);
    LegendLabel->setText(
        "<b>Legenda</b><br>"
        "<span style='color:rgb(220,50,50);'>&#9644;</span> deo deadlock ciklusa<br>"
        "<span style='color:rgb(160,160,160);'>&#9644;</span> par postoji, nije prijavljen<br>"
        "A &#8594; B: drzi A, pokusava B");
    LegendLabel->setStyleSheet(
        "background-color: rgba(255,255,255,220); "
        "border: 1px solid #999; "
        "border-radius: 4px; "
        "padding: 6px; "
        "font-size: 9pt;");
    LegendLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
    LegendLabel->adjustSize();
    LegendLabel->raise();
}

void GraphView::resizeEvent(QResizeEvent *event) {
    QGraphicsView::resizeEvent(event);
    RepositionLegend();
}

void GraphView::RepositionLegend() {
    const int Margin = 10;
    int X = viewport()->width() - LegendLabel->width() - Margin;
    int Y = viewport()->height() - LegendLabel->height() - Margin;
    LegendLabel->move(X, Y);
}

void GraphView::SetGraph(const QVector<QPair<QString, QString>> &Edges,
                          const QSet<QString> &CycleEdges) {
    Scene->clear();

    if (Edges.isEmpty()) {
        QGraphicsTextItem *EmptyText = Scene->addText("Nema parova zakljucavanja za prikaz.");
        EmptyText->setPos(0, 0);
        return;
    }

    QVector<QString> Nodes;
    QSet<QString> Seen;
    for (const auto &Edge : Edges) {
        if (!Seen.contains(Edge.first)) {
            Seen.insert(Edge.first);
            Nodes.append(Edge.first);
        }
        if (!Seen.contains(Edge.second)) {
            Seen.insert(Edge.second);
            Nodes.append(Edge.second);
        }
    }

    const double Radius = 150.0;
    const double NodeRadius = 25.0;
    QMap<QString, QPointF> Positions;
    int N = Nodes.size();
    for (int i = 0; i < N; i++) {
        double Angle = 2.0 * M_PI * i / N;
        double X = Radius * std::cos(Angle);
        double Y = Radius * std::sin(Angle);
        Positions[Nodes[i]] = QPointF(X, Y);
    }

    // Crta svaku ivicu kao zakrivljen luk (kvadratna Bezier kriva), ne
    // pravu liniju. 
    for (const auto &Edge : Edges) {
        QPointF From = Positions[Edge.first];
        QPointF To = Positions[Edge.second];

        QString EdgeKey = Edge.first + "->" + Edge.second;
        bool HasReverse = Edges.contains({Edge.second, Edge.first});

        QPointF Direction = To - From;
        double Length = std::sqrt(Direction.x() * Direction.x() + Direction.y() * Direction.y());
        QPointF UnitDir = (Length > 0) ? Direction / Length : QPointF(1, 0);
        QPointF Normal(-UnitDir.y(), UnitDir.x());

        // Koliko se luk "izbocuje" u stranu - samo ako postoji povratna
        // ivica (inace ostaje prava linija, nema potrebe za zakrivljenjem).
        const double CurveAmount = HasReverse ? 35.0 : 0.0;
        QPointF Bulge = Normal * CurveAmount;

        // Mala razlika u pravcu Normal-a (isti pravac kao i sam luk) - da tacka
        // gde ivica dodiruje krug bude malo pomerena, i da se ivice u SUPROTNIM
        // smerovima (npr. m1->m2 i m2->m1) ne sreku na ISTOJ tacki kruga.
        const double NodeOffset = 6.0;
        QPointF StartEdge = From + UnitDir * NodeRadius + Normal * NodeOffset;
        QPointF EndEdge = To - UnitDir * NodeRadius + Normal * NodeOffset;
        QPointF MidPoint = (StartEdge + EndEdge) / 2.0 + Bulge;

        bool IsCycleEdge = CycleEdges.contains(EdgeKey);
        QColor Color = IsCycleEdge ? QColor(220, 50, 50) : QColor(160, 160, 160);
        QPen Pen(Color);
        Pen.setWidth(IsCycleEdge ? 3 : 1);

        QPainterPath Path(StartEdge);
        Path.quadTo(MidPoint, EndEdge);

        QGraphicsPathItem *Arc = Scene->addPath(Path, Pen);
        Arc->setZValue(0);

        // Strelica na kraju - pravac je tangenta krive u krajnjoj tacki,
        // sto je (EndEdge - MidPoint) normalizovano.
        QPointF TangentDir = EndEdge - MidPoint;
        double TangentLen = std::sqrt(TangentDir.x() * TangentDir.x() + TangentDir.y() * TangentDir.y());
        QPointF ArrowDir = (TangentLen > 0) ? TangentDir / TangentLen : UnitDir;
        QPointF ArrowNormal(-ArrowDir.y(), ArrowDir.x());

        const double ArrowSize = 10.0;
        QPointF ArrowP1 = EndEdge - ArrowDir * ArrowSize + ArrowNormal * (ArrowSize * 0.5);
        QPointF ArrowP2 = EndEdge - ArrowDir * ArrowSize - ArrowNormal * (ArrowSize * 0.5);

        QPolygonF ArrowHead;
        ArrowHead << EndEdge << ArrowP1 << ArrowP2;

        QGraphicsPolygonItem *Arrow = Scene->addPolygon(ArrowHead, Pen, QBrush(Color));
        Arrow->setZValue(0);
    }

    for (const QString &Node : Nodes) {
        QPointF Pos = Positions[Node];

        QGraphicsEllipseItem *Circle = Scene->addEllipse(
            Pos.x() - NodeRadius, Pos.y() - NodeRadius,
            NodeRadius * 2, NodeRadius * 2,
            QPen(Qt::black), QBrush(QColor(230, 230, 250)));
        Circle->setZValue(1);

        QGraphicsTextItem *Label = Scene->addText(Node);
        QRectF TextRect = Label->boundingRect();
        Label->setPos(Pos.x() - TextRect.width() / 2, Pos.y() - TextRect.height() / 2);
        Label->setZValue(2);
    }

    Scene->setSceneRect(Scene->itemsBoundingRect().adjusted(-30, -30, 30, 30));

}