#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QFileSystemWatcher>
#include <QMainWindow>
#include <QTextBrowser>
#include "dbmanager.h"
#include "dir.h"
#include "file.h"
#include "forpostdialog.h"
#include <QDebug>
#include "hashthread.h"
#include "dirmonitor.h"
class FilesView;
class PartMetFile;
class TrafficLights;
class FreenetClipboard;
class FreenetWindow;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    DbManager* dbm;
    void storeFiles();
    void loadFiles();
    void updateAll();
    void fillFileList();
    void fillFileList1();
    void fillDir(const wchar_t* dir);
    inline File* findFileBySDN(quint64 size, quint64 sec, const QString & name);
    inline File* findFileBySN(quint64 size, const QString & name);
    QVector<File*> savedFiles, files;
    QList<QVector<File*>> searchFiles;
    void clearFiles(QVector <File*> & _files);

    FilesView* setFilesTable(int ntab, QVector<File*>* _files);
//    FilesView* mainView;
    QList <FilesView*> views;
    void tableHeaderPressed(int);
    ForpostDialog * forpostDialog;
    quint64 ed2kSize(const QString& s);
    QString ed2kHash(const QString& s);
    int searchFileBySize(quint64 sz);
    int searchFileByMD5(const QString &md5);
    int searchFileByEd2k(const QString & ed2k);
    QVector<PartMetFile*> partMetFiles;
    void fillPartFiles();
    void Log(const QString & s);
    QStringList logList;
    QString logString;
    QFileSystemWatcher  watcher;
    TrafficLights * trafficLights;
    void storeFile(File* f, bool deleteDeleted = false);
    void addStoreFile(File* f);
    void deleteZeroSizeFiles();
    int fid;
    void commit();
    HashThread ed2kThread, md5Thread;
    void startEd2kHashing();
    void stopEd2kHashing();
    void startMD5Hashing();
    void stopMD5Hashing();
    void activateFileRow(int nf);
    void timerEvent(QTimerEvent*) override;
    bool busy;
    QList<DirMonitorThread*> dirMonitors;
    bool addFile(const QString & fn);
    bool ready;
    QMutex mutex;
    FreenetClipboard* freenetClipboard;
    FreenetWindow* freenetWindow;
    void findDuplicates();
    void findDupSize();
    void showDuplicates();
    void findDupMsecs();

private slots:
    void on_actionUpdate_All_triggered();

    void on_actionTest_triggered();
    void onClipboardChanged();
    void on_actionTest1_triggered();
    void on_actionLog_triggered();
    void onDirChanged(DirMonitorThread*);
//    void FileChanged(const QString &path);
    void on_actionClearLog_triggered();
    void onHashed(bool eflag, int nf);
    void on_actionEd2k_triggered();

    void on_actionMD5_triggered();
    void ed2kThreadFinished();
    void md5ThreadFinished();
    void on_actionFreenet_clipboard_triggered();

    void on_actionFreenet_Window_triggered();

    void on_actionFind_duplicates_triggered();

    void on_actionShow_Duplicates_triggered();

    void on_actionMsecs_triggered();

    void on_actionSize_triggered();
public slots:
    void onLog(const QString& s);

private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
