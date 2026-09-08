#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "AnalyzerRunner.h"

class QPlainTextEdit;
class QTextEdit;
class QLabel;

class MainWindow : public QMainWindow {
public:
    MainWindow();

private:
    QPlainTextEdit *CodeEditor;
    QLabel *ResultLabel;
    QTextEdit *ResultText;
    AnalyzerRunner Runner;

    void OnOpenFile();
    void OnAnalyze();
    void DisplayResult(const AnalysisResult &Result);
};

#endif

