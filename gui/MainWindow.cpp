#include "MainWindow.h"
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

MainWindow::MainWindow() {
    setWindowTitle("Race/Deadlock Analyzer");
    resize(900, 650);

    QWidget *Central = new QWidget(this);
    QVBoxLayout *MainLayout = new QVBoxLayout(Central);

    QHBoxLayout *ButtonLayout = new QHBoxLayout();
    QPushButton *OpenButton = new QPushButton("Ucitaj fajl", Central);
    QPushButton *AnalyzeButton = new QPushButton("Analiziraj", Central);
    ButtonLayout->addWidget(OpenButton);
    ButtonLayout->addWidget(AnalyzeButton);
    ButtonLayout->addStretch();

    CodeEditor = new QPlainTextEdit(Central);
    CodeEditor->setPlaceholderText("Otkucaj C kod ovde, ili klikni 'Ucitaj fajl'...");
    QFont MonoFont("Monospace");
    MonoFont.setStyleHint(QFont::TypeWriter);
    CodeEditor->setFont(MonoFont);

    ResultLabel = new QLabel("Nema jos rezultata.", Central);
    ResultText = new QTextEdit(Central);
    ResultText->setReadOnly(true);

    QSplitter *Splitter = new QSplitter(Qt::Horizontal, Central);
    Splitter->addWidget(CodeEditor);

    QWidget *ResultWidget = new QWidget();
    QVBoxLayout *ResultLayout = new QVBoxLayout(ResultWidget);
    ResultLayout->addWidget(ResultLabel);
    ResultLayout->addWidget(ResultText);
    ResultLayout->setContentsMargins(0, 0, 0, 0);
    Splitter->addWidget(ResultWidget);

    MainLayout->addLayout(ButtonLayout);
    MainLayout->addWidget(Splitter);

    setCentralWidget(Central);

    connect(OpenButton, &QPushButton::clicked, this, &MainWindow::OnOpenFile);
    connect(AnalyzeButton, &QPushButton::clicked, this, &MainWindow::OnAnalyze);
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
}