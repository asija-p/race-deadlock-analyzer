#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "AnalyzerRunner.h"
#include "LineNumberEditor.h"

class QTextEdit;
class QLabel;
class GraphView;
class QTableWidget;

class MainWindow : public QMainWindow {
public:
    MainWindow();

private:
    LineNumberEditor *CodeEditor;
    QLabel *ResultLabel;
    QTextEdit *ResultText;
    GraphView *Graph;
    QTableWidget *RaceTable;
    AnalyzerRunner Runner;

    void OnOpenFile();
    void OnAnalyze();
    void DisplayResult(const AnalysisResult &Result);
    void OnRaceRowClicked(int row, int column);
    void HighlightLines(int line1, int line2);
};

#endif