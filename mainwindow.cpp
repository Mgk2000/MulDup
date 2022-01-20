#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "dir.h"
#include "file.h"
# include "filesview.h"
#include <QDebug>
#include <QClipboard>
#include <QDesktopWidget>

#include <QDirIterator>
#include <QSqlQuery>
#include <QSqlError>
#include <QElapsedTimer>
#include <QMimeData>
#include <QScrollBar>
#include <QFileSystemWatcher>
#include <QScreen>
#include <QFileDialog>
#include "filterform.h"
#include "hash.h"
#include "ed2k.h"
#include "trafficlights.h"
#include "dirmonitor.h"
#include "freenetclipboard.h"
#include "freenetwindow.h"
#include "showpreviewdialog.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
     ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setMainWin(this);
    QScreen *screen = QGuiApplication::primaryScreen();
    QRect  screenGeometry = screen->geometry();
    int w = screenGeometry.width();
    int h = screenGeometry.height();
    setGeometry(x(), y(), w *2 / 3, h * 2/ 3 );
    int ww = QGuiApplication::primaryScreen()->geometry().width();
    int fs = 9;
    switch(ww)
    {
    case 1920:
        fs = 9; break;
    case 2160:
        fs = 10; break;
    case 3840:
        fs = 11; break;
    }
    ui->textBrowser->setFontPointSize(fs);

    dbm = new DbManager;
    ready = false;
    dbm->open();
    fillvideoExts();
    files.reserve(100000);
//    for (int i=1; i>=0; i--)
    for (int i=0; i<2; i++)
        views.append(setFilesTable(i, &files));
//    mainView = setFilesTable(0, &files);
    ui->tabWidget->tabBar()->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->tabWidget->tabBar(), SIGNAL(customContextMenuRequested(const QPoint &)), SLOT(onTabContextMenuRequested(const QPoint &)));
    forpostDialog = new ForpostDialog(0);
    freenetClipboard = new FreenetClipboard(0);
    freenetWindow = new FreenetWindow(0);
    if( QClipboard* c = QApplication::clipboard() )
    {
        QObject::connect( c, SIGNAL( dataChanged() ), SLOT( onClipboardChanged() ) );
    }
    trafficLights = new TrafficLights(0);
    ui->textBrowser->setMaximumHeight(QGuiApplication::primaryScreen()->geometry().width() / 12);
    qInstallMessageHandler(msgHandler);
    busy = false;
    //========================================
    connect(&hashThread, SIGNAL(hashed(bool, int)), this, SLOT(onHashed(bool, int)));
    connect(&hashThread, SIGNAL(finished()), this, SLOT(hashThreadFinished()));
    connect (&hashThread, SIGNAL (logSignal(const QString&, bool)), this, SLOT(onLog(const QString&, bool)));
    connect (&hashThread, SIGNAL (hashingFailed()), this, SLOT(onHashingFailed()));
    dirMonitors.append(new DirMonitorThread(this, "U:\\Y"));
    if (dirXExists())
        dirMonitors.append(new DirMonitorThread(this, "X:\\Z"));
    for (int i=0; i< dirMonitors.count(); i++)
    connect (dirMonitors[i], SIGNAL(dirChanged(DirMonitorThread*)), this, SLOT(onDirChanged(DirMonitorThread*)));
    for (int i=0; i< dirMonitors.count(); i++)
        dirMonitors[i]->start();
    startTimer(1000);
    autoHashing = false;
    maxTime = 9223372036854775;
    ui->actionAuto_hashing->setCheckable(true);
    dirToMove = "U:/Y";
    showPreviewDialog = new ShowPreviewDialog(this);

}

MainWindow::~MainWindow()
{

    for (int i=0; i< dirMonitors.count(); i++)
        dirMonitors[i]->terminate();
    delete freenetClipboard;
    delete freenetWindow;
    delete ui;
}

void MainWindow::storeFiles()
{
    for (int i =0; i< files.length(); i++)
        if (files[i]->id != i+1)
            qDebug() << i;
    QSqlQuery query;
    query.exec("begin transaction");
    query.exec("delete from files");
    auto rc = query.exec();
    if (!rc)
        qDebug() << "delete error:"
                        << query.lastError();
    query.prepare(QString("insert into files (name, size, birth, MD5,  fexists, ed2k, forpost, id) ")+
     "values (:name, :size, :birth, :MD5, :fexists, :ed2k, :forpost, :id)");
    for (int i=0; i< files.count(); i++)
    {
        File* f = files[i];
        query.bindValue(0, f->name);
        query.bindValue(1, f->size);
        qint64 t = f->birth.toMSecsSinceEpoch();
//        qDebug() << t;
        query.bindValue(2, f->birth.toMSecsSinceEpoch());
        query.bindValue(3, f->MD5);
        query.bindValue(4, f->exists);
        query.bindValue(5, f->ed2k);
        query.bindValue(6, f->forpost);
        query.bindValue(7, f->id);
        auto rc = query.exec();
        if (!rc)
        {
            qDebug() << f->name;
            qDebug() << "insert error:"
                            << query.lastError();
        }
    }
    query.exec("commit");


}

void MainWindow::loadFiles()
{
    QSqlQuery query("SELECT name, size, birth, MD5,  fexists, ed2k, forpost FROM files");
    while (query.next())
    {
        File * f = new File();
        f->name = query.value(0).toString();
        f->size = query.value(1).toULongLong();
        f->birth.setMSecsSinceEpoch( query.value(2).toULongLong());
        f->MD5 = query.value(3).toString();
        f->exists = query.value(4).toBool();
        f->ed2k = query.value(5).toString();
        f->forpost = query.value(6).toString();
        files.append(f);
        qDebug() << f->name << f->size << f->birth << f->exists;
    }

}

void MainWindow::updateAll()
{
    busy = true;
    autoHashing = false;
    hashThread.stop();
    for (int i=0; i< dirMonitors.count(); i++)
        dirMonitors[i]->stop();
    QApplication::setOverrideCursor(Qt::WaitCursor);
    QApplication::processEvents();
    QElapsedTimer timer;
    qint64 t0, t1, t2, t3;
    timer.start();
    clearFiles(files);
    t0 =  timer.elapsed();
    qInfo() << "Enumerating existing videos...";
    fillFileList1();
    t1 =  timer.elapsed();
    qInfo() << "Updating database...";
    QSqlQuery query("SELECT name, size, birth, MD5, fexists, ed2k, forpost FROM files where size>0");
    while (query.next())
    {
        File * f = new File();
        f->name = query.value(0).toString();
        if (f->name.contains("140141"))
            qDebug() << "140141";
        f->size = query.value(1).toULongLong();
        qint64 msec = query.value(2).toULongLong();
        f->birth.setMSecsSinceEpoch( msec );
        f->MD5 = query.value(3).toString();
        f->exists = query.value(4).toBool();
        f->ed2k = query.value(5).toString();
        if (!f->exists)
        {
            if (f->size < 512*1024)
            {
                delete f;
                continue;
            }
            File* f1 = findFileBySN(f->size, f->name);
            if (f1)
            {
                copyFileInfo(f1, f);
                delete f;
                continue;
            }
            else
            {
                files.append(f);
                f->id = ++fid;
            }
        }
        else
        {
            f->forpost = query.value(6).toString();
            File* f1 = findFileBySDN(f->size, msec, f->name);
            if (!f1)
            {
                 f->exists = false;
                 f->id = ++fid;
                 files.append(f);
            }
            else
            {
                copyFileInfo(f1, f);
                continue;
            }
//            continue;
        }
        //qDebug() << f->name << f->size << f->birth << f->dirind << f->exists;
    }
    t2 =  timer.elapsed();
/*    for  (int i=0; i< files.count(); i++)
        if (files[i]->birth > QDateTime::currentDateTime())
            files[i]->birth = QDateTime::fromString("01.01.2021 00:00:00");*/
    storeFiles();
    t3 =  timer.elapsed();
    qDebug() << "t0, t1, t2, t3=" << t0 << t1 << t2 << t3;
  //  mainView->setFiles(&files);
  //  mainView->refresh();
    for (int i =0; i< views.count(); i++)
    {
        views[i]->setFiles(&files);
        views[i]->refresh();
    }
    QApplication::restoreOverrideCursor();
    QApplication::processEvents();
    qInfo() << "Ready. " << files.count() << "files.";
    ready = true;
    for (int i=0; i< dirMonitors.count(); i++)
        dirMonitors[i]->resume();
//    storeDirs();
    autoHashing = ui->actionAuto_hashing->isChecked();
    busy = false;
}


void MainWindow::fillFileList1()
{
    QElapsedTimer timer;
    timer.start();
    quint64 t1 =0, t2 =0;
    fid =0;
    clearFiles(files);
    for (int i=0; i< dirMonitors.count(); i++)
        fillDir((const wchar_t *)dirMonitors[i]->dir.utf16());
    //fillDir((const wchar_t *)QString("U:/Y").utf16());
    t1 = timer.elapsed();
    qInfo() << "Enumerating .part files...";
    QApplication::processEvents();

    fillPartFiles();
    for (int i=0; i< partMetFiles.length(); i++)
    {
        PartFile* pf = new PartFile(partMetFiles[i]);
        pf->id = ++fid;
        files.append(pf);
    }
    t2 = timer.elapsed();
    qDebug() << "fillFileList1: t1, t2=" << t1 << t2;

}

void MainWindow::fillDir(const wchar_t *dir)
{
    QString dirName = (QString::fromWCharArray(dir).replace('/', '\\') + '\\');
    HANDLE h;
    wchar_t ffn[1000];
    wcscpy(ffn, dir);
    wcscat(ffn, L"\\*.*");
    WIN32_FIND_DATAW findData;
    h=FindFirstFileW(ffn, &findData);
    while(true)
    {
        QString fn = QString::fromWCharArray(findData.cFileName) ;
//        qDebug() << fn;
        if (fn == "." || fn == "..")
            goto nextfile;
        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            wchar_t nextDir[1000];
            wcscpy(nextDir, dir);
            wcscat(nextDir, L"\\");
            wcscat(nextDir, findData.cFileName);
            fillDir(nextDir);
        }
        else
        {
            if (!isVideo(fn))
                goto nextfile;
            File* fi = new File;
            fi->name = fn;
            wchar_t nextFile[1000];
            wcscpy(nextFile, dir);
            wcscat(nextFile, L"\\");
            wcscat(nextFile, findData.cFileName);
            QString fullfn = QString::fromWCharArray(nextFile);
            QFileInfo * fii = fi->info = new QFileInfo(fullfn);
            fi->size = fii->size();
            fi->birth = fii->birthTime();
            fi->exists = true;
            fi->id = ++fid;
            files.append(fi);
            if (fid % 100 == 0)
            {
                qInfo() << fid;
                QApplication::processEvents();
            }
        }
        nextfile:
        bool bf = FindNextFileW(h,&findData);
        if (!bf)
            break;

    }
}

File* MainWindow::findFileBySDN(quint64 size, quint64 msec, const QString & name)
{
    for (QVector<File*>::ConstIterator it = files.begin();
         it != files.end(); it++)
    {
        File * f = *it;
        if ((f->size == size && f->birth.toMSecsSinceEpoch() == (qint64)msec)  &&
           ( f->name == name))
            return f;
    }
    return nullptr;
}

File *MainWindow::findFileBySN(quint64 size, const QString &name)
{
    for (QVector<File*>::ConstIterator it = files.begin();
         it != files.end(); it++)
    {
        File * f = *it;
        if (f->size == size  && ( f->name == name))
            return f;
    }
    return nullptr;
}

void MainWindow::copyFileInfo(File *fdst, File *fsrc)
{
    if (fdst->MD5.trimmed() == "" && fsrc->MD5.trimmed() != "")
        fdst->MD5 = fsrc->MD5;
    if (fdst->ed2k.trimmed() == "" && fsrc->ed2k.trimmed() != "")
        fdst->ed2k = fsrc->ed2k;
    if (fdst->forpost.trimmed() == "" && fsrc->forpost.trimmed() != "")
        fdst->forpost = fsrc->forpost;
}


void MainWindow::clearFiles(QVector<File *> &_files)
{
    QVector<File*>::iterator fi;
    for (fi = _files.begin(); fi!= _files.end(); fi++)
    {
        delete *fi;
    }
    _files.clear();
}

void MainWindow::on_actionUpdate_All_triggered()
{
    updateAll();
}

FilesView* MainWindow::setFilesTable(int ntab, QVector<File *>* _files)
{
    QWidget * w = ui->tabWidget->widget(ntab);
    FilterForm* ff = new FilterForm(0/*w*/);
     FilesView* fw = new FilesView(w ,ff, _files);
    QVBoxLayout *viewLayout = new QVBoxLayout(w);
    viewLayout->setParent(w);
    viewLayout->setMargin(0);
    viewLayout->setSpacing(0);
    viewLayout->addWidget(fw);
    ff->setMinimumHeight(92);
    viewLayout->addWidget(ff);
    return fw;
}

void MainWindow::tableHeaderPressed(int)
{

}


bool MainWindow::addFile(const QString &fn)
{
    QFileInfo * fi = new QFileInfo(fn);
    if (!fi->exists())
    {
        delete fi;
        return false;
    }
    File * f = new File;
    f->info = fi;
    f->name = fi->fileName();
    f->size = fi->size();
    f->birth = fi->birthTime();
    f->exists = true;
    f->id = files.length() +1;
    files.append(f);
    return true;
}

void MainWindow::findDuplicates()
{
    for (int i=0; i< files.count(); i++)
        files[i]->duplicate = false;
    for (int i=0; i< files.count(); i++)
        for (int j=i+1; j< files.count(); j++)
            if ((files[i]->exists || files[i]->isPartFile) && (files[j]->exists || files[j]->isPartFile)
                    && files[i]->size == files[j]->size)
            {
                    bool bdup = true;
                    if (files[i]->MD5 != "" && files[j]->MD5 != "" && files[i]->MD5 != files[j]->MD5)
                        bdup = false;
                    if (bdup)
                        files[i]->duplicate = files[j]->duplicate = true;
            }

}

void MainWindow::findDupSize()
{
    for (int i=0; i< files.count(); i++)
        files[i]->duplicate = false;
    for (int i=0; i< files.count(); i++)
        for (int j=i+1; j< files.count(); j++)
            if ((files[i]->exists || files[i]->isPartFile) && (files[j]->exists || files[j]->isPartFile)
                    && abs((qint64)files[i]->size - (qint64)files[j]->size) < 10)
            {
                        files[i]->duplicate = files[j]->duplicate = true;
            }


}

void MainWindow::showDuplicates()
{
    views[0]->fmodel.showDuplicates();
}

void MainWindow::findDupMsecs()
{

    for (int i=0; i< files.count(); i++)
        files[i]->duplicate = false;
    for (int i=0; i< files.count(); i++)
    {
        if (!files[i]->info|| files[i]->isPartFile)
            continue;
        for (int j=i+1; j< files.count(); j++)
            if (!files[j]->info || files[j]->isPartFile)
                continue;
            else if (files[i]->info->birthTime().toMSecsSinceEpoch() == files[j]->info->birthTime().toMSecsSinceEpoch())
            {
                qint64 t1 = files[i]->info->birthTime().toMSecsSinceEpoch();
                qint64 t2 = files[j]->info->birthTime().toMSecsSinceEpoch();
                qDebug() << "----------------------------------";
                qDebug() << files[i]->filePath() << files[i]->info->birthTime();
                qDebug() << files[j]->filePath() << files[j]->info->birthTime();
                qDebug() << "t1, t2=" << t1 << t2;
                files[i]->duplicate = files[j]->duplicate = true;
            }
    }
}

void MainWindow::copyHashFromExistent(File *file)
{
    for (int i=0; i< files.count(); i++)
    {
        if (files[i] != file && files[i]->size == file->size && file->name == files[i]->name)
        {
            if (file->MD5 == "" && files[i]->MD5 != "")
                file->MD5 = files[i]->MD5;
            if (file->ed2k == "" && files[i]->ed2k != "")
                file->ed2k = files[i]->ed2k;
        }
    }
}

void MainWindow::fillLiatFromtable(const QString &table, QVector<File *> &ffiles)
{
     //QSqlQuery query(QString("select * from %1").arg(table));
     QSqlQuery query(QString("SELECT name, size, birth, MD5, fexists, ed2k, forpost FROM %1 where size>0").arg(table));
     while (query.next())
     {
         File * f = new File();
         f->name = query.value(0).toString();
         f->size = query.value(1).toULongLong();
         qint64 msec = query.value(2).toULongLong();
         f->birth.setMSecsSinceEpoch( msec );
         f->MD5 = query.value(3).toString();
         f->exists = query.value(4).toBool();
         f->ed2k = query.value(5).toString();
         f->forpost = query.value(6).toString();
         ffiles << f;
     }
}

void MainWindow::adjustFilesFromCopy()
{
    QVector<File*> copyFiles, newFiles;;
    fillLiatFromtable("copyfiles", copyFiles);
    qDebug() << "copyFiles=" << copyFiles.count();
//add to files if not exists
    for (int j=0; j< copyFiles.count(); j++)
    {
        File * cf = copyFiles[j];
        {
            for (int i =0; i< files.count(); i++)
            if (files[i]->name == cf->name && files[i]->size == cf->size)
            {
                if (files[i]->MD5 == "")
                    files[i]->MD5 = cf->MD5;
                if (files[i]->ed2k == "")
                    files[i]->ed2k = cf->ed2k;
                if (files[i]->forpost == "")
                    files[i]->forpost = cf->forpost;
                goto nexti;
            }
        File* f = new File(*cf);
        f->exists = false;
        f->info = nullptr;
        newFiles << f;
        }
        nexti:;
    }
    qDebug() << "Files=" << files.count();
    files << newFiles;
    qDebug() << "Files=" << files.count();
    for (int j=0; j< copyFiles.count(); j++)
        delete copyFiles[j];
    storeFiles();

}

bool MainWindow::dirXExists()
{
    QFile f("x:/z/x01/czechchildrensdays.mp4");
    if (!f.exists())
        return false;
    if (!f.open(QIODevice::ReadOnly))
        return false;
    char buf;
    int nb = f.read(&buf,1);
    if (nb<0)
        return false;
    return true;
}

int MainWindow::newTab()
{
    QWidget* newTab = new QWidget(ui->tabWidget);
    int nt = ui->tabWidget->count();
    nt = ui->tabWidget->addTab(newTab, QString("Tab %1").arg(nt));
    FilesView* view = setFilesTable(nt, &files);
    views.append(view);
    return nt;
}

void MainWindow::setTabText(const QString &s)
{
    ui->tabWidget->setTabText(ui->tabWidget->currentIndex(), s);
}

void MainWindow::moveFile(File* file)
{
    QFileDialog fd;
    fd.setDirectory(dirToMove);
    fd.setFileMode(QFileDialog::Directory);
    //fd.setOption(QFileDialog::ShowDirsOnly);
    auto rc= fd.exec();
    if (rc == QDialog::Accepted)
    {
        ignoringFiles.clear();
        ignoringFiles << file->name;
        QString newDir = fd.directory().path();
        qDebug() << newDir;
        QString newfn =  newDir + "/" + file->name;
        bool rc = QFile::rename(file->filePath(), newfn);
        if (rc)
        {
            file->setFileName(newfn);
            Log(file->name + " moved to " + fd.directory().path());
        }
        else
            Log ("move unsuccessful");
        dirToMove = fd.directory().path();
    }

}

void MainWindow::showPreview(File *file)
{
    QString preview = file->forpost + "/" + file->name + ".jpg";
    showPreview(preview);
}

void MainWindow::showPreview(const QString &preview)
{
    showPreviewDialog->run(preview);
}

quint64 MainWindow::ed2kSize(const QString &s)
{
    QString sbeg = s.mid(0, 13);
    if (sbeg != "ed2k://|file|")
            return 0;
    int ind = s.indexOf('|', 14);
    int ind2 = s.indexOf('|', ind+1);
    QString ss = s.mid(ind+1, ind2-ind-1);
    return ss.toLongLong();
}

QString MainWindow::ed2kHash(const QString &s)
{
    QString sbeg = s.mid(0, 13);
    if (sbeg != "ed2k://|file|")
            return "";
    int ind = s.indexOf('|', 14);
    int ind2 = s.indexOf('|', ind+1);
    QString ss = s.mid(ind2+1, 32);
    return ss;

}

int MainWindow::searchFileBySize(quint64 sz)
{
    for (int i  =0; i< files.count(); i++)
        if (files[i]->size == sz)
            return i;
    return -1;
}

int MainWindow::searchFileByMD5(const QString &md5)
{
    QString ml = md5.toLower();
    for (int i  =0; i< files.count(); i++)
        if (files[i]->MD5.toLower() == ml)
            return i;
    return -1;
}

int MainWindow::searchFileByEd2k(const QString &ed2k)
{
    QString el = ed2k.toLower();
    for (int i  =0; i< files.count(); i++)
        if (files[i]->ed2k.toLower() == el)
            return i;
    return -1;
}

void MainWindow::checkFilesIds()
{
    for (int i=0; i< files.count(); i++)
    {
        if (files[i]->id != i+1)
        {
            QString es = QString ("Ids violation. Files[%1] = %2").arg(i).arg(files[i]->id);
            qDebug() << es;
            Log(es);
            return;
        }
    }
    Log("Ids OK");
}

void MainWindow::activateFileRow( int nf)
{
    File* f = files[nf];
    qDebug() << nf << f->name;
    ui->tabWidget->setCurrentIndex(0);
    views[0]->activateFile(f);
}

void MainWindow::timerEvent(QTimerEvent *)
{
    if (autoHashing && lastAddedTime != maxTime)
    {
        quint64 now = QDateTime::currentDateTime().toSecsSinceEpoch();
        if (now - lastAddedTime > 5 && !hashThread.isRunning())
            startHashing();
    }
}

void MainWindow::addIgnoringFiles(const QString &fn)
{
    ignoringFiles << fn;
}

void MainWindow::clearIgnoringFiles()
{
    ignoringFiles.clear();
}

void MainWindow::onClipboardChanged()
{
if( QClipboard* c = QApplication::clipboard() )
{
    qDebug() << "Clipboard" << c->text();
    if( const QMimeData* m = c->mimeData() )
        if( m->hasText() )
        {
            QString s = m->text();
            if (s.mid(0,7) == "MulDup:")
            {
                QString ss = s.mid(7,1000);
                qDebug() << "MulDup " << s;
                //views[0]->searchBySize(ss);
                bool ok;
                qint64 sz = ss.toULongLong(&ok);
                if (!ok)
                {
                    Log("invali size: "+ss);
                    return;
                }
                views[views.count()-1]->searchBySize(sz);
                return;
            }
            if (s.length() == 32)
            {
                QByteArray ba = s.toLocal8Bit();
                QByteArray ha = QByteArray::fromHex(ba);
                QString ss = ha.toHex();
                if (s.toLower() ==ss)
                {
                    int nf = searchFileByEd2k(s);
                    if (nf<0)
                        nf = searchFileByMD5(s);
                    if (nf>=0)
                    {
                        activateFileRow(nf);
                        trafficLights->showLight(2);
                        return;
                    }

                }
            }
            QString ed2k = ed2kHash(s).toLower();
            if (!ed2k.isEmpty())
            {
                qInfo() << ed2k;
/*                int nf = searchFileByEd2k(ed2k);
                if (nf>=0)
                {
                    File* f = files[nf];
                    qDebug() << nf << f->name;
                    ui->tabWidget->setCurrentIndex(0);
                    views[0]->activateFile(f);
                    trafficLights->showLight(2);
                    return;
                }*/

                quint64 esize = ed2kSize(s);
    //            qDebug() << "ed2k=" << esize;
                views[views.count()-1]->searchBySize(esize);

                if (esize >0)
                {
                   int nf = searchFileBySize(esize);
                   if (nf>=0)
                   {
                       //activateFileRow(nf);
                       trafficLights->showLight(1);
                   }
                   else
                       trafficLights->showLight(0);
                }
            }
        }
        else
        {
            qDebug() << "MimeData=" << m;
        }
}
}



void MainWindow::fillPartFiles()
{
    partMetFiles.clear();
    QStringList exts;
    exts << "*.part";
    QDir pDir(partDir());
    QStringList metFiles= pDir.entryList(exts);
    for (int i=0; i< metFiles.count(); i++)
    {
        PartMetFile * pf = new PartMetFile(metFiles[i]);
        if (pf->name != "")
            partMetFiles << pf;
        else delete pf;
    }
}

void MainWindow::Log(const QString & s, bool newLine)
{
    if (!newLine)
        ui->textBrowser->setPlainText(ui->textBrowser->toPlainText().append(" " + s));
    else
        ui->textBrowser->append(s);
    QScrollBar *sb = ui->textBrowser->verticalScrollBar();
    sb->setValue(sb->maximum());
}

void MainWindow::storeFile(File *f, bool deleteDeleted)
{
    QSqlQuery query;
    QString s = QString("update files set md5=\"%1\", ed2k=\"%2\", forpost=\"%3\",fexists=%4,name=\"%5\",size=%6  where id=%7").
            arg(f->MD5).arg(f->ed2k).arg(f->forpost).arg(f->exists).arg(f->name).arg(f->size).arg(f->id);
    auto rc = query.exec(s);
    if (!rc)
        qDebug() << "update error:"
                        << query.lastError();
    if (deleteDeleted && f->exists)
    {
        QString lname = f->name.toLower();
        s=QString("delete from files where lower(name)=\"%1\" and size=%2 and fexists=0").arg(lname).arg(f->size);
        qDebug() << s;
        rc=query.exec(s);
        if (!rc)
            qDebug() << "StoreFile: delete error:" << query.lastError();
    }

}

void MainWindow::addStoreFile(File *f)
{
    QSqlQuery query;
    QString s = QString("insert into files (id, name, fexists, size, birth) values (%1, \"%2\", %3, %4, %5)").
            arg(f->id).arg(f->name).arg(f->exists).arg(f->size).arg(f->birth.toMSecsSinceEpoch());
    auto rc = query.exec(s);
    if (!rc)
        qDebug() << "addStoreFile: insert error:" << query.lastError();
}

void MainWindow::deleteZeroSizeFiles()
{
    QSqlQuery query ("delete from files where size<500000");
}

void MainWindow::commit()
{
    dbm->commit();
}

void MainWindow::startHashing()
{
    if (hashThread.running)
        return;
    hashThread.fillFiles();
    if (!hashThread.files.count())
    {
        lastAddedTime = maxTime;
        return;
    }
    for (int i=0; i< hashThread.files.count(); i++)
    {
        qDebug() << "copyHashFromExistent " << hashThread.files[i]->name;
        copyHashFromExistent(hashThread.files[i]);
    }
    hashThread.start();
    ui->actionHash->setChecked(true);
}

void MainWindow::stopHashing()
{
    if (hashThread.running)
        hashThread.stop();
    ui->actionHash->setChecked(false);

}

void MainWindow::on_actionTest_triggered()
{
/*    QElapsedTimer timer;
    timer.start();
    QVector <QString> files;
//    QDirIterator it ("U:/Y", videoFilters, QDir::Files | QDir::NoSymLinks, QDirIterator::Subdirectories);
    QDirIterator it ("C:/Users",  QDir::Files | QDir::NoSymLinks, QDirIterator::Subdirectories);
    while (it.hasNext())
        files.append(it.next());
    qDebug()<< "Elapsed=" <<  timer.elapsed() << "files=" << files.length();*/
//    deleteZeroSizeFiles();

}

void MainWindow::on_actionTest1_triggered()
{
    QDir dir("X:/Z");
    qDebug() << dir.exists();
    qDebug() << dir.path();
    qDebug() << dir.absolutePath();
    qDebug() << dir.canonicalPath();
    qDebug() << dir.count();
    QStringList sl = dir.entryList();
    qDebug() << sl;
    QDirIterator it (dir, QDirIterator::Subdirectories);
    while (it.hasNext())
    {
          qDebug() << it.next();

          // /etc/.
          // /etc/..
          // /etc/X11
          // /etc/X11/fs
          // ...
      }
//    fillFileList1();
//    qDebug() << "fillFileList1 Files " << files.count();

}

void MainWindow::on_actionLog_triggered()
{
    ui->textBrowser->setVisible(!ui->textBrowser->isVisible());
    ui->actionLog->setChecked(ui->textBrowser->isVisible());
}

void MainWindow::onDirChanged( DirMonitorThread* dirMonitor)
{
    bool changed = false;
    mutex.lock();
    dirMonitor->onDirChangeRunning = true;
    qDebug() << "onDirChanged started changes=" << dirMonitor->changedFiles.length() << " adds=" << dirMonitor->addedFiles.count();
//    dirMonitor.handleEvents();
    qDebug() << "OnDirChanged 1 addedFiles.count() = " << dirMonitor->addedFiles.count();
    for (int i =0 ; i< dirMonitor->addedFiles.count(); i++)
    {
        QString fn = QFileInfo(dirMonitor->addedFiles[i]).fileName();
        QString ffn = QFileInfo(dirMonitor->addedFiles[i]).filePath();
        for (int j=0; j< dirMonitor->deletedFiles.count(); j++)
        {
            QString dfn = QFileInfo(dirMonitor->deletedFiles[j]).fileName();
            QString fdfn = QFileInfo(dirMonitor->deletedFiles[j]).filePath();
            if (fn == dfn)
            {
                qDebug() << "Moved " << fn;
                for (int k = 0; k< files.count(); k++)
                    if(files[k]->exists  && files[k]->filePath() == fdfn)
                    {
                        files[k]->setFileName(ffn);
                        //storeFile(files[k]);
                        for (int l=0; l< dirMonitor->changedFiles.count(); l++)
                            if (dirMonitor->changedFiles[l] == fdfn)
                            {
                                dirMonitor->changedFiles[l]="";
                                break;
                            }
                        goto nextAdded;
                    }
            }
        }
        {
        File * f = new File(dirMonitor->addedFiles[i]);
        f->id = files.count()+ 1;
        files.append(f);
        addStoreFile(f);
        qDebug() << "Added" << f->name;
        qInfo() << "Added" << f->name << " -> " << f->info->absoluteDir().path();
        }
        nextAdded:
        changed = true;
    }
    qDebug() << "OnDirChanged 2 changedFiles.count() = " << dirMonitor->changedFiles.count();
    for (int i =0 ; i< dirMonitor->changedFiles.count(); i++)
    {
        if (dirMonitor->changedFiles[i].isEmpty())
            continue;
        for (int j =files.count()-1; j>=0 ; j--)
        {
//            qDebug() << "OnDirChanged 2.1 " << files[j]->filePath();
//            qDebug() << "OnDirChanged 2.2 " << dirMonitor->changedFiles[i];
            if (dirMonitor->changedFiles[i] == files[j]->filePath())
            {
                File* f = files[j];
                f->setFileName(files[j]->filePath()); //update info;
                if (f->size==0)
                {
                    qDebug() << "OnDirChanged 2.3 " << files[j]->filePath();

                }
                if (!f->exists)
                    qInfo() << "Deleted" << f->name;
                storeFile(f);
                changed = true;
                goto nexti;
            }
        }
        qDebug() << "Not found changed file " << dirMonitor->changedFiles[i];
        nexti:;
    }
    qDebug() << "OnDirChanged 3 renamedFiles.count() = " << dirMonitor->renamedFiles.count();
    for (int i =0 ; i< dirMonitor->renamedFiles.count(); i++)
        for (int j =0; j< files.count(); j++)
        {
            File* f = files[j];
            if (f->info && f->filePath() == dirMonitor->renamedFiles[i].oldName)
            {
                f->info->setFile(dirMonitor->renamedFiles[i].newName);
                f->name = files[j]->info->fileName();
            }
        }
    qDebug() << "OnDirChanged 4";
    dirMonitor->addedFiles.clear();
    dirMonitor->changedFiles.clear();
    dirMonitor->deletedFiles.clear();
    dirMonitor->renamedFiles.clear();
    if (changed)
        lastAddedTime = QDateTime::currentSecsSinceEpoch();
    mutex.unlock();
    dirMonitor->onDirChangeRunning = false;

    qDebug() << "onDirChanged ended";
    for (int i=0; i< views.count(); i++)
        views[i]->setFiles(&files, false);
    qDebug() << "onDirChanged setFiles";

    //    mainView->refresh();
}

void MainWindow::on_actionClearLog_triggered()
{
    ui->textBrowser->clear();
}

void MainWindow::onHashed(bool /*eflag*/, int nf)
{
    storeFile(files[nf]);
    for (int i=0; i< views.count(); i++)
        views[i]->refresh();
}

void MainWindow::onHashingFailed()
{
    lastAddedTime = QDateTime::currentSecsSinceEpoch();
}


void MainWindow::on_actionHash_triggered()
{
    if (!hashThread.running)
        startHashing();
    else
        stopHashing();
//    ui->actionMD5->setChecked(md5Thread.running);

}

void MainWindow::hashThreadFinished()
{
    qInfo() << "hashThreadFinished()";
    for (int i=0; i< hashThread.files.count(); i++)
        storeFile(hashThread.files[i]);
    ui->actionHash->setChecked(false);
    lastAddedTime = maxTime;
}


void MainWindow::on_actionFreenet_clipboard_triggered()
{
    freenetClipboard->show();
    freenetClipboard->readClipboard();
}

void MainWindow::on_actionFreenet_Window_triggered()
{
    freenetWindow->show();
    freenetWindow->readClipboard();
}

void MainWindow::on_actionFind_duplicates_triggered()
{
    findDuplicates();
}

void MainWindow::on_actionShow_Duplicates_triggered()
{
    showDuplicates();
    for (int i=0; i< views.count(); i++)
        views[i]->refresh();
}

void MainWindow::on_actionMsecs_triggered()
{
    findDupMsecs();
}

void MainWindow::on_actionSize_triggered()
{
    findDupSize();
}

void MainWindow::onLog(const QString &s, bool newLine)
{
    Log(s, newLine);
}

void MainWindow::onTabContextMenuRequested(const QPoint & p)
{
    if (p.isNull())
    {
        qDebug() << "null";
        return;
    }
    int ntab = ui->tabWidget->tabBar()->tabAt(p);
    qDebug() << "Tab Menu " << ntab;
    QMenu menu;
    menu.addAction("New Tab");
    menu.addAction("Duplicate Tab");
    if (ntab > 1)
        menu.addAction("Close Tab");
    QAction* act = menu.exec(ui->tabWidget->tabBar()->mapToGlobal(p));
    if (!act)
        return;
    qDebug() << act->text();
    if (act->text() == "New Tab")
        newTab();
}

void MainWindow::on_actionSave_table_triggered()
{
    QSqlQuery query("delete from copyfiles");
    query.exec();
    query.exec("insert into copyfiles select * from files");
}


void MainWindow::on_actionAuto_hashing_triggered()
{
    autoHashing = ! autoHashing;
    ui->actionAuto_hashing->setChecked(autoHashing);
}

void MainWindow::on_actionAdjust_From_Copy_triggered()
{
    adjustFilesFromCopy();
}

void MainWindow::on_actionStore_Files_triggered()
{
    storeFiles();
}

void MainWindow::on_actionCkeck_Ids_triggered()
{
    checkFilesIds();
}
