#ifndef DIRMONITOR_H
#define DIRMONITOR_H

#include <QThread>
#include <QObject>
#include <QMutex>
#include <windows.h>
class MainWindow;
struct RenamedFile
{
    QString oldName, newName;
};

class DbManager;

class DirMonitorThread : public QThread
{
    Q_OBJECT
public:
    DirMonitorThread(MainWindow* mw,const QString & _dir);
    virtual ~DirMonitorThread();
    void run() override;
    void stop();
    void resume();
    void WatchDirectory();
    void handleEvents();
    bool HandleChangeEvent(FILE_NOTIFY_INFORMATION* event);
    HANDLE dirh;
    uint8_t * change_buf;
    bool running;
    QMutex& mutex();
    QString renamedFrom;
    QString renamedDir;
    bool handleRet;
    RenamedFile renamedFile;
    QVector <QString> addedFiles, changedFiles;
    QVector <RenamedFile> renamedFiles;
    volatile bool onDirChangeRunning;
    QString dir;
    MainWindow* mainWin;
signals:
    void dirChanged(DirMonitorThread*);
};
//Q_DECLARE_METATYPE(DirMonitorThread)
#endif // DIRMONITOR_H
