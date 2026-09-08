#include "AnalyzerRunner.h"
#include <QProcess>
#include <QTemporaryFile>
#include <QFile>
#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>

AnalysisResult AnalyzerRunner::Run(const QString &code) {
    AnalysisResult Result;

    if (code.trimmed().isEmpty()) {
        Result.ErrorMessage = "Editor je prazan - otkucaj ili ucitaj kod prvo.";
        return Result;
    }

    // Sacuvaj kod u privremeni .c fajl, jer analyzer ocekuje putanju do
    // fajla, ne kod na stdin.
    QTemporaryFile TempFile("XXXXXX.c");
    TempFile.setAutoRemove(false);  // brisemo rucno posle, da izbegnemo trke
    if (!TempFile.open()) {
        Result.ErrorMessage = "Ne mogu da napravim privremeni fajl.";
        return Result;
    }
    TempFile.write(code.toUtf8());
    QString TempPath = TempFile.fileName();
    TempFile.close();

    QString AnalyzerPath = QCoreApplication::applicationDirPath() + "/../analyzer";

    QProcess Process;
    Process.start(AnalyzerPath, QStringList() << "--json" << TempPath);

    if (!Process.waitForFinished(10000)) {
        Result.ErrorMessage = "Analyzer se nije zavrsio na vreme.";
        QFile::remove(TempPath);
        return Result;
    }

    Result.RawOutput = QString(Process.readAllStandardOutput());
    Result.RawError = QString(Process.readAllStandardError());

    QFile::remove(TempPath);

    QJsonParseError ParseError;
    QJsonDocument Doc = QJsonDocument::fromJson(Result.RawOutput.toUtf8(), &ParseError);

    if (ParseError.error != QJsonParseError::NoError) {
        Result.ErrorMessage = "Greska pri parsiranju JSON-a: " + ParseError.errorString();
        return Result;
    }

    QJsonObject Root = Doc.object();
    Result.AllPairs = Root["all_pairs"].toArray(); 
    Result.Deadlocks = Root["deadlocks"].toArray();
    Result.Races = Root["races"].toArray();
    Result.Success = true;

    return Result;
}