#include "hashthread.h"
#include "mainwindow.h"
#include "mainwindow.h"
HashThread::HashThread(bool _flagEd2k) : running(false), flagEd2k(_flagEd2k)
{
}

void HashThread::run()
{
    running = true;
    log("Hashing started...");

    for (int i=0; i< files.length() && running; i++)
    {
        if (!files[i]->info)
            continue;
        qDebug() << "Hashing " << i << files[i]->filePath();
        log(QString("Hashing %1 %2").arg(i).arg(files[i]->filePath()));
        if (flagEd2k )
        {
            if (files[i]->ed2k == "")
            files[i]->ed2k = getEd2k(files[i]->canonicalFilePath());
        }
        else
        { if (files[i]->MD5 == "")
            files[i]->MD5 = getHash(files[i]->canonicalFilePath(), QCryptographicHash::Md5);
        }
        log(QString("Hashed %1").arg(files[i]->name));
    }
    log("Hashing complete.");
    running = false;
}

void HashThread::fillFiles()
{
    files.clear();
    for (int i=0; i< mainWin()->files.length(); i++)
    {
        if (mainWin()->files[i]->exists && !mainWin()->files[i]->isPartFile)
        {
            if (flagEd2k)
            {
                if (mainWin()->files[i]->ed2k == "")
                    files.append(mainWin()->files[i]);
            }
            else
            {
                if (mainWin()->files[i]->MD5 == "")
                    files.append(mainWin()->files[i]);
            }
        }
    }
    qInfo() << "Files to hash:" << files.count();
    qDebug() << "Files to hash:" << files.count();
}

void HashThread::log(const QString &s)
{
    emit logSignal(s);
}

