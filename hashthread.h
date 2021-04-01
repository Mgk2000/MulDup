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
    HashThread(bool _flagEd2k);
    void run() override;
    void stop()
        { running = false;}

    void  fillFiles();
    void log(const QString& s);
    volatile bool running;
    QVector<File*> files;
private:
    bool flagEd2k;
signals:
    void hashed(bool eflag, int nf);
    void logSignal(const QString&);
};

#endif // HASHTHREAD_H
