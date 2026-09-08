#ifndef LINENUMBEREDITOR_H
#define LINENUMBEREDITOR_H

#include <QPlainTextEdit>

class QWidget;
class LineNumberArea;

// QPlainTextEdit prosiren brojevima linija sa leve strane - standardan Qt
// obrazac (widget koji se iscrtava pored editora, prati njegov scroll i
// broj linija).
class LineNumberEditor : public QPlainTextEdit {
    Q_OBJECT

public:
    explicit LineNumberEditor(QWidget *parent = nullptr);

    void LineNumberAreaPaintEvent(QPaintEvent *event);
    int LineNumberAreaWidth();

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void UpdateLineNumberAreaWidth(int newBlockCount);
    void UpdateLineNumberArea(const QRect &rect, int dy);

private:
    QWidget *LineNumberArea;
};

#endif