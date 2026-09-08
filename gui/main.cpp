#include <QApplication>
#include <QFile>
#include <QTextStream>
#include "widgets/MainWindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setStyle("Fusion");

    QFile StyleFile(":/style.qss");
    if (StyleFile.open(QFile::ReadOnly | QFile::Text)) {
        QTextStream Stream(&StyleFile);
        app.setStyleSheet(Stream.readAll());
    }

    MainWindow window;
    window.show();

    return app.exec();
}