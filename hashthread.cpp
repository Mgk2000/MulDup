#include "hashthread.h"
#include "mainwindow.h"
#include "mainwindow.h"
HashThread::HashThread() : running(false)
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
        if (files[i]->ed2k == "")
        {
            if (isMulDir(files[i]->info->absolutePath()))
            {
                for (int j =0; j<files.count(); j++)
                    if (files[j]->isPartFile && files[j]->size == files[i]->size && files[j]->name == files[i]->name)
                    {
                        log ("Copy ed2k hash from .part file");
                        files[i]->ed2k = files[j]->ed2k;
                        break;
                    }
            }
            if (files[i]->ed2k == "")
            {
                log(QString("Hashing ed2k %1 %2...").arg(i).arg(files[i]->filePath()));
                files[i]->ed2k = getEd2k(files[i]->canonicalFilePath());
            }
            log("Done", false);
        }
        if (files[i]->MD5 == "")
        {
            log(QString("Hashing md5 %1 %2...").arg(i).arg(files[i]->filePath()));
            files[i]->MD5 = getHash(files[i]->canonicalFilePath(), QCryptographicHash::Md5);
            log("Done", false);
        }
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
                if (mainWin()->files[i]->ed2k == "" || mainWin()->files[i]->MD5 == "")
                    files.append(mainWin()->files[i]);
        }
    }
    qInfo() << "Files to hash:" << files.count();
    qDebug() << "Files to hash:" << files.count();
}

void HashThread::log(const QString &s, bool newLine)
{
    emit logSignal(s, newLine);
}

