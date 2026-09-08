#include "LineNumberEditor.h"
#include <QPainter>
#include <QTextBlock>

class LineNumberAreaWidget : public QWidget {
public:
    explicit LineNumberAreaWidget(LineNumberEditor *editor)
        : QWidget(editor), Editor(editor) {}

    QSize sizeHint() const override {
        return QSize(Editor->LineNumberAreaWidth(), 0);
    }

protected:
    void paintEvent(QPaintEvent *event) override {
        Editor->LineNumberAreaPaintEvent(event);
    }

private:
    LineNumberEditor *Editor;
};

LineNumberEditor::LineNumberEditor(QWidget *parent) : QPlainTextEdit(parent) {
    LineNumberArea = new LineNumberAreaWidget(this);

    connect(this, &QPlainTextEdit::blockCountChanged, this, &LineNumberEditor::UpdateLineNumberAreaWidth);
    connect(this, &QPlainTextEdit::updateRequest, this, &LineNumberEditor::UpdateLineNumberArea);

    UpdateLineNumberAreaWidth(0);
}

int LineNumberEditor::LineNumberAreaWidth() {
    int Digits = 1;
    int Max = qMax(1, blockCount());
    while (Max >= 10) {
        Max /= 10;
        Digits++;
    }
    return 12 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * Digits;
}

void LineNumberEditor::UpdateLineNumberAreaWidth(int /*newBlockCount*/) {
    setViewportMargins(LineNumberAreaWidth(), 0, 0, 0);
}

void LineNumberEditor::UpdateLineNumberArea(const QRect &rect, int dy) {
    if (dy) {
        LineNumberArea->scroll(0, dy);
    } else {
        LineNumberArea->update(0, rect.y(), LineNumberArea->width(), rect.height());
    }

    if (rect.contains(viewport()->rect())) {
        UpdateLineNumberAreaWidth(0);
    }
}

void LineNumberEditor::resizeEvent(QResizeEvent *event) {
    QPlainTextEdit::resizeEvent(event);

    QRect cr = contentsRect();
    LineNumberArea->setGeometry(QRect(cr.left(), cr.top(), LineNumberAreaWidth(), cr.height()));
}

void LineNumberEditor::LineNumberAreaPaintEvent(QPaintEvent *event) {
    QPainter painter(LineNumberArea);
    painter.fillRect(event->rect(), QColor(240, 240, 240));

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            QString number = QString::number(blockNumber + 1);
            painter.setPen(QColor(120, 120, 120));
            painter.drawText(0, top, LineNumberArea->width() - 6, fontMetrics().height(),
                              Qt::AlignRight, number);
        }

        block = block.next();
        top = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
        blockNumber++;
    }
}