#include "MainWindow.h"
#include "GraphView.h"
#include "LineNumberEditor.h"
#include <QTableWidgetItem>
#include <QTextCursor>
#include <QTextBlock>
#include <QTextEdit>
#include <QTableWidget>
#include <QPushButton>
#include <QTextEdit>
#include <QPlainTextEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>
#include <QFileDialog>
#include <QLabel>
#include <QSplitter>
#include <QFile>
#include <QJsonObject>
#include <QJsonArray>
#include <QHeaderView>

MainWindow::MainWindow() {
    setWindowTitle("Race/Deadlock Analyzer");
    resize(1000, 800);

    QWidget *Central = new QWidget(this);
    QVBoxLayout *MainLayout = new QVBoxLayout(Central);

    QHBoxLayout *ButtonLayout = new QHBoxLayout();
    QPushButton *OpenButton = new QPushButton("Ucitaj fajl", Central);
    QPushButton *AnalyzeButton = new QPushButton("Analiziraj", Central);
    ButtonLayout->addWidget(OpenButton);
    ButtonLayout->addWidget(AnalyzeButton);
    ButtonLayout->addStretch();

    CodeEditor = new LineNumberEditor(Central);
    CodeEditor->setPlaceholderText("Otkucaj C kod ovde, ili klikni 'Ucitaj fajl'...");
    QFont MonoFont("Monospace");
    MonoFont.setStyleHint(QFont::TypeWriter);
    CodeEditor->setFont(MonoFont);

    ResultLabel = new QLabel("Nema jos rezultata.", Central);
    ResultText = new QTextEdit(Central);
    ResultText->setReadOnly(true);
    ResultText->setMaximumHeight(150);  // tekst ostaje kompaktan

    Graph = new GraphView(Central);

    RaceTable = new QTableWidget(0, 4, Central);
    RaceTable->setHorizontalHeaderLabels({"Promenljiva", "Ozbiljnost", "Nit 1 (linija)", "Nit 2 (linija)"});
    RaceTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    RaceTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    RaceTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    RaceTable->setMaximumHeight(150);

    // Gornji splitter: editor levo, tekst rezultata + graf desno.
    QWidget *RightSide = new QWidget();
    QVBoxLayout *RightLayout = new QVBoxLayout(RightSide);
    RightLayout->setContentsMargins(0, 0, 0, 0);
    RightLayout->addWidget(ResultLabel);
    RightLayout->addWidget(ResultText);
    RightLayout->addWidget(RaceTable);
    RightLayout->addWidget(Graph, /*stretch=*/1);  // graf uzima vecinu prostora
    

    QSplitter *Splitter = new QSplitter(Qt::Horizontal, Central);
    Splitter->addWidget(CodeEditor);
    Splitter->addWidget(RightSide);
    Splitter->setStretchFactor(0, 1);
    Splitter->setStretchFactor(1, 2);  // desna strana (rezultat+graf) sira

    MainLayout->addLayout(ButtonLayout);
    MainLayout->addWidget(Splitter);

    setCentralWidget(Central);

    connect(OpenButton, &QPushButton::clicked, this, &MainWindow::OnOpenFile);
    connect(AnalyzeButton, &QPushButton::clicked, this, &MainWindow::OnAnalyze);
    connect(RaceTable, &QTableWidget::cellClicked, this, &MainWindow::OnRaceRowClicked);
}

void MainWindow::OnOpenFile() {
    QString FilePath = QFileDialog::getOpenFileName(
        this, "Izaberi C fajl", QString(), "C fajlovi (*.c)");

    if (FilePath.isEmpty()) {
        return;
    }

    QFile File(FilePath);
    if (!File.open(QIODevice::ReadOnly | QIODevice::Text)) {
        ResultLabel->setText("Ne mogu da otvorim fajl: " + FilePath);
        return;
    }

    CodeEditor->setPlainText(QString::fromUtf8(File.readAll()));
}

void MainWindow::OnAnalyze() {
    AnalysisResult Result = Runner.Run(CodeEditor->toPlainText());
    DisplayResult(Result);
}

void MainWindow::DisplayResult(const AnalysisResult &Result) {
    if (!Result.Success) {
        ResultLabel->setText(Result.ErrorMessage);
        ResultText->setPlainText("Sirovi izlaz analizatora:\n" + Result.RawOutput +
                                  "\n\nStderr:\n" + Result.RawError);
        Graph->SetGraph({}, {});

        return;
    }

    QString Summary = QString("Deadlock-ovi: %1  |  Race parovi: %2")
                           .arg(Result.Deadlocks.size())
                           .arg(Result.Races.size());
    ResultLabel->setText(Summary);

    QString Details;

    if (Result.Deadlocks.isEmpty()) {
        Details += "Nema deadlock-a.\n\n";
    } else {
        Details += "=== DEADLOCK CIKLUSI ===\n";
        for (const QJsonValue &Val : Result.Deadlocks) {
            QJsonArray Cycle = Val.toObject()["cycle"].toArray();
            QStringList Edges;
            for (const QJsonValue &Edge : Cycle) {
                Edges << Edge.toString();
            }
            Details += "  " + Edges.join(" -> ") + "\n";
        }
        Details += "\n";
    }

    if (Result.Races.isEmpty()) {
        Details += "Nema race uslova.\n";
    } else {
        Details += "=== RACE PAROVI ===\n";
        for (const QJsonValue &Val : Result.Races) {
            QJsonObject Race = Val.toObject();
            QString VarName = Race["var"].toString();
            QString Severity = Race["severity"].toString();
            QJsonArray Pair = Race["pair"].toArray();
            QJsonObject A = Pair[0].toObject();
            QJsonObject B = Pair[1].toObject();

            Details += QString("  [%1] %2: linija %3 (%4) <-> linija %5 (%6)\n")
                           .arg(Severity, VarName,
                                QString::number(A["line"].toInt()), A["thread"].toString(),
                                QString::number(B["line"].toInt()), B["thread"].toString());
        }
    }

    ResultText->setPlainText(Details);

    // Pripremi podatke za GraphView: sve ivice, i skup "from->to" stringova
    // koji su deo bilo kog prijavljenog ciklusa (za crvenu boju).
    QVector<QPair<QString, QString>> Edges;
    for (const QJsonValue &Val : Result.AllPairs) {
        QJsonObject Pair = Val.toObject();
        Edges.append({Pair["from"].toString(), Pair["to"].toString()});
    }

    QSet<QString> CycleEdges;
    for (const QJsonValue &Val : Result.Deadlocks) {
        QJsonArray Cycle = Val.toObject()["cycle"].toArray();
        for (const QJsonValue &Edge : Cycle) {
            CycleEdges.insert(Edge.toString());
        }
    }

    Graph->SetGraph(Edges, CycleEdges);

        RaceTable->setRowCount(0);
        for (const QJsonValue &Val : Result.Races) {
            QJsonObject Race = Val.toObject();
            QString VarName = Race["var"].toString();
            QString Severity = Race["severity"].toString();
            QJsonArray Pair = Race["pair"].toArray();
            QJsonObject A = Pair[0].toObject();
            QJsonObject B = Pair[1].toObject();

            int Row = RaceTable->rowCount();
            RaceTable->insertRow(Row);

            QTableWidgetItem *VarItem = new QTableWidgetItem(VarName);
            QTableWidgetItem *SeverityItem = new QTableWidgetItem(Severity);
            QTableWidgetItem *Thread1Item = new QTableWidgetItem(
                QString("%1 (linija %2)").arg(A["thread"].toString()).arg(A["line"].toInt()));
            QTableWidgetItem *Thread2Item = new QTableWidgetItem(
                QString("%1 (linija %2)").arg(B["thread"].toString()).arg(B["line"].toInt()));

            // Boja pozadine reda prema ozbiljnosti - MUST-RACE (sigurno opasno)
            // crvenkasto, MAY-RACE (neizvesno) zuckasto.
            QColor RowColor = (Severity == "MUST")
                                ? QColor(255, 210, 210)
                                : QColor(255, 240, 190);

            VarItem->setBackground(RowColor);
            SeverityItem->setBackground(RowColor);
            Thread1Item->setBackground(RowColor);
            Thread2Item->setBackground(RowColor);

            RaceTable->setItem(Row, 0, VarItem);
            RaceTable->setItem(Row, 1, SeverityItem);
            RaceTable->setItem(Row, 2, Thread1Item);
            RaceTable->setItem(Row, 3, Thread2Item);

            RaceTable->item(Row, 0)->setData(Qt::UserRole, A["line"].toInt());
            RaceTable->item(Row, 0)->setData(Qt::UserRole + 1, B["line"].toInt());
        }
}

void MainWindow::OnRaceRowClicked(int row, int /*column*/) {
    QTableWidgetItem *Item = RaceTable->item(row, 0);
    if (!Item) return;

    int Line1 = Item->data(Qt::UserRole).toInt();
    int Line2 = Item->data(Qt::UserRole + 1).toInt();
    HighlightLines(Line1, Line2);
}

void MainWindow::HighlightLines(int line1, int line2) {
    QList<QTextEdit::ExtraSelection> Selections;

    auto AddHighlight = [&](int LineNumber) {
        QTextEdit::ExtraSelection Selection;
        Selection.format.setBackground(QColor(255, 235, 150));
        Selection.format.setProperty(QTextFormat::FullWidthSelection, true);

        QTextCursor Cursor(CodeEditor->document()->findBlockByLineNumber(LineNumber - 1));
        Cursor.clearSelection();
        Selection.cursor = Cursor;

        Selections.append(Selection);
    };

    AddHighlight(line1);
    AddHighlight(line2);

    CodeEditor->setExtraSelections(Selections);

    QTextCursor ScrollCursor(CodeEditor->document()->findBlockByLineNumber(line1 - 1));
    CodeEditor->setTextCursor(ScrollCursor);
    CodeEditor->centerCursor();
}