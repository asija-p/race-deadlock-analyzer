#ifndef ANALYZERRUNNER_H
#define ANALYZERRUNNER_H

#include <QString>
#include <QJsonArray>

// Rezultat jednog pokretanja analyzer-a - cisto podaci, nema Qt Widgets
// zavisnosti. Ako parsiranje ili pokretanje ne uspe, Success je false i
// ErrorMessage objasnjava sta se desilo (RawOutput/RawError se prosledjuju
// za debug prikaz).
struct AnalysisResult {
    bool Success = false;
    QString ErrorMessage;
    QString RawOutput;
    QString RawError;
    QJsonArray Deadlocks;
    QJsonArray Races;
};

// Pokrece analyzer --json na datom C kodu (kod se privremeno cuva u fajl,
// jer analyzer ocekuje putanju, ne stdin) i vraca parsiran rezultat.
class AnalyzerRunner {
public:
    // Putanja do analyzer izvrsnog fajla se odredjuje relativno u odnosu na
    // direktorijum gui izvrsnog fajla (build/gui/ -> ../analyzer).
    AnalysisResult Run(const QString &code);
};

#endif