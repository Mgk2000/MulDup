#include "dirmonitor.h"
#include <windows.h>
#include "mainwindow.h"
#include "hash.h"
#include "mainwindow.h"
#include <QDebug>
#include "dbmanager.h"
#include "mainwindow.h"
#include <strsafe.h>
bool DirMonitorThread::HandleChangeEvent(FILE_NOTIFY_INFORMATION* event)
{
    DWORD name_len = event->FileNameLength / sizeof(wchar_t);
    QString fn = QString::fromWCharArray(event->FileName, name_len);
    if (mainWin->ignoringFiles.contains(QFileInfo(fn).fileName()))
        return true;
    qDebug() << "HandleChangeEvent fn=" << fn;
    qDebug() << "isVideo=" << isVideo(fn) << "action=" << event;
    if (isVideo(fn))
    {
        QString fullFn = QString(dir + "/" + fn).replace('\\', '/');
        QString lowFn = fullFn.toLower();
        switch (event->Action)
        {
        case FILE_ACTION_ADDED:
            qDebug() << "Added " << QString::fromWCharArray(event->FileName, name_len);
            addedFiles.append(fullFn);
            return true;
        case FILE_ACTION_REMOVED:
            qDebug() << "Removed:" << QString::fromWCharArray(event->FileName, name_len);
            changedFiles.append(fullFn);
            deletedFiles.append(fullFn);
            return true;
        case FILE_ACTION_MODIFIED:
        {
            qDebug() << "Modified:" << QString::fromWCharArray(event->FileName, name_len);
            for (int i=0; i< addedFiles.count(); i++)
                if (fullFn == addedFiles[i])
                    return false;
            for (int i=0; i< changedFiles.count(); i++)
                if (fullFn == changedFiles[i])
                    return false;
            changedFiles.append(fullFn);
            return true;
        }
        case FILE_ACTION_RENAMED_OLD_NAME:
            qDebug() << "Renamed from:" << QString::fromWCharArray(event->FileName, name_len);
            renamedFile.oldName = fullFn;
            return false;
        case FILE_ACTION_RENAMED_NEW_NAME:
            qDebug() << "Renamed to:" << QString::fromWCharArray(event->FileName, name_len);
            renamedFile.newName = fullFn;
            renamedFiles.append(renamedFile);
            return false;

        default:
            qDebug() << "Unknown action!" << QString::fromWCharArray(event->FileName, name_len);
            return false;
    }
    }
    return false;
}

QMutex &DirMonitorThread::mutex()
{
    return mainWin->mutex;
}
DirMonitorThread::DirMonitorThread(MainWindow * mw,const QString& _dir): dirh(0), onDirChangeRunning(false), dir(_dir),  mainWin(mw)
{
}

DirMonitorThread::~DirMonitorThread()
{
    quit();
}

void DirMonitorThread::run()
{
    running = true;
    int sz = 1024 * 4096;
    change_buf = new uint8_t [sz];

    dirh = CreateFileA(dir.toStdString().c_str(),
        FILE_LIST_DIRECTORY,
        FILE_SHARE_READ /*| FILE_SHARE_WRITE | FILE_SHARE_DELETE*/,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS  | FILE_ATTRIBUTE_READONLY/*| FILE_FLAG_OPEN_NO_RECALL*/,
        NULL);

    if (dirh == INVALID_HANDLE_VALUE)
    {
        fprintf(stderr, "Failed to open directory");
        char  pszFunction[20] = "createfile";
        LPVOID lpMsgBuf;
        LPVOID lpDisplayBuf;
        DWORD dw = GetLastError();

        FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            dw,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPTSTR) &lpMsgBuf,
            0, NULL );

        // Display the error message and exit the process

        lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT,
            (lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)pszFunction) + 40) * sizeof(TCHAR));
        StringCchPrintf((LPTSTR)lpDisplayBuf,
            LocalSize(lpDisplayBuf) / sizeof(TCHAR),
            TEXT("%s failed with error %d: %s"),
            pszFunction, dw, lpMsgBuf);
        MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK);

        LocalFree(lpMsgBuf);
        LocalFree(lpDisplayBuf);
        running = false;
        return;

    }
    handleRet = false;
    WatchDirectory();

}

void DirMonitorThread::stop()
{
    running = false;
}

void DirMonitorThread::resume()
{
    running = true;
}

void DirMonitorThread::WatchDirectory()
{
    DWORD change_len;
    BOOL succ;

    for (;;)
    {
         succ = ReadDirectoryChangesW(
            dirh, change_buf, 1024 * 4096, TRUE,
            FILE_NOTIFY_CHANGE_FILE_NAME  |
            FILE_NOTIFY_CHANGE_DIR_NAME   |
            FILE_NOTIFY_CHANGE_LAST_WRITE,
            &change_len, NULL, NULL
            );
        mutex().lock();
        qDebug()  << "DirMonitor read start succ, len==" << succ << change_len;
        if (!succ)
        {
            qDebug() <<  "Failed to read directory changes";
        }
        while (onDirChangeRunning);
        handleEvents();
        mutex().unlock();
        emit dirChanged(this);
    }
}

void DirMonitorThread::handleEvents()
{
    FILE_NOTIFY_INFORMATION *event;
    event = (FILE_NOTIFY_INFORMATION*)change_buf;

    for (;;)
    {
        qDebug() << " running =" << running;
        if (running)
        {
           bool b = HandleChangeEvent(event);
           handleRet = handleRet || b;
        }

        // Are there more events to handle?
        if (event->NextEntryOffset)
            *((uint8_t**)&event) += event->NextEntryOffset;
        else
            break;
    }
    qDebug()  << "DirMonitor read end Ret=" << handleRet;
    handleRet = false;

}
