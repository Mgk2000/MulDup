#ifndef HASHTHREAD_H
#define HASHTHREAD_H
#include "hash.h"
#include <QThread>
#include <QObject>
#include <QMutex>

class HashThread : public QThread
{
    Q_OBJECT
public:
    HashThread();
    void run() override;
    void stop()
        { running = false;}

    void  fillFiles();
    void log(const QString& s, bool newLine = true);
    volatile bool running;
    QVector<File*> files;
signals:
    void hashed(bool eflag, int nf);
    void logSignal(const QString&, bool newLine);
};

#endif // HASHTHREAD_H
