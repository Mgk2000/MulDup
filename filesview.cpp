#include "filesview.h"
#include "file.h"
#include "dir.h"
#include "mainwindow.h"
#include <QHeaderView>
#include <QDebug>
#include <QMenu>
#include <QProcess>
#include <QDir>
#include <QMessageBox>
#include <QCryptographicHash>
#include <QFileDialog>
#include <QSqlQuery>
#include <QScreen>
#include <QSqlError>
#include <QTimer>
#include <QClipboard>
#include <QGuiApplication>
#include "mainwindow.h"
#include "filterform.h"
#include "hash.h"
#include "ed2k.h"

FilesView::FilesView( QWidget* parent,FilterForm *  ff,  QVector<File *> *_files) :
    QTableView(nullptr/*parent*/),
    fmodel( _files), filterForm(ff)
{
//    filterForm->setParent(this);
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
    setFont(QFont("Times",fs));
    setParent(parent);
    fmodel.view = this;
    this->setModel(&fmodel);
    verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    verticalHeader()->setDefaultSectionSize(9);
    verticalHeader()->setVisible(false);
    horizontalHeader()->setStretchLastSection(true) ;
    setColumnWidth(0, mainWin()->width() / 2);
    horizontalHeader()->setMinimumSectionSize(30);
    setColumnWidth(3, 150);
    setColumnWidth(4, 150);
    setColumnWidth(5, 150);

    connect (horizontalHeader(), SIGNAL(sectionPressed(int)), this, SLOT(headerPressed(int)));
    //horizontalHeader()->setHighlightSections(false);
    setShowGrid(false);
    setSortingEnabled(true);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    fillOpenWithActions();
    connect(filterForm->searchButton(), SIGNAL(clicked()), this, SLOT(filterSearchPressed()) );
    connect(filterForm->appendButton(), SIGNAL(clicked()), this, SLOT(filterAppendPressed()) );
    connect(filterForm->discardButton(), SIGNAL(clicked()), this, SLOT(filterDiscardPressed()) );
    connect(filterForm->unsortButton(), SIGNAL(clicked()), this, SLOT(unsortPressed()) );
    connect(filterForm->bytesButton(), SIGNAL(clicked()), this, SLOT(bytesClicked()) );
    connect(filterForm->mbytesButton(), SIGNAL(clicked()), this, SLOT(mbytesClicked()) );
    connect(filterForm->birthCheckBox(), SIGNAL(clicked()), this, SLOT(birthClicked()) );
    connect(filterForm->showExistingCheckBox(), SIGNAL(clicked()), this, SLOT(onRefresh()) );
    connect(filterForm->showDeletedCheckBox(), SIGNAL(clicked()), this, SLOT(onRefresh()) );
}

void FilesView::refresh()
{
    fmodel.refresh();
//    for (int i=0; i< 100 /*files->count()*/;  i++)
//        setRowHeight(i, 12);

    //this->setModel(&model);
}

void FilesView::setFiles(QVector<File *> *_files, bool immed)
{
    fmodel.setFiles(_files, immed);
}
void FilesView::mousePressEvent(QMouseEvent *event)
{
    qDebug() << event;
    QPoint pos = event->pos();
    QPersistentModelIndex index = indexAt(pos);
    currRow = index.row();
    qDebug() << index.row() << index.column();
    if (currRow <0 || index.column() <0)
        return;
    if(event->button() == Qt::RightButton)
    {
        if(index.row() >=0)
            contextMenu(index.row(), pos);
        //emit activated(index);
    }
    else
        QTableView::mousePressEvent(event);
}

void FilesView::contextMenu(int row, const QPoint &pos)
{
    File * file = fmodel.files[row];
//select row
    QModelIndex index = fmodel.index(row, 0);
//    Then you can set that modelindex as the current one using
    setCurrentIndex(index);
    QMenu menu;
    if (!file->exists && !file->isPartFile )
    {
        QAction deleteAction("Delete from base");
        menu.addAction(&deleteAction);
        QAction * act = menu.exec(mapToGlobal(pos));
        if (act == &deleteAction)
        {
            deleteDeleted(file);
            fmodel.files.remove(row);
            fmodel.refresh();
        }
        return;
    }
    QString fname = file->canonicalFilePath();
    QAction explorerAct("Open with Explorer");
    menu.addAction(&explorerAct);
    QMenu playMenu("Play");
    menu.addMenu(&playMenu);
    QAction vlcAct("VLC");
    playMenu.addAction(&vlcAct);
    QAction mpcAct("MPC");
    playMenu.addAction(&mpcAct);
    QAction potAct("PotPlayer");
    playMenu.addAction(&potAct);
    QMenu openWithMenu("Open with");
    menu.addMenu(&openWithMenu);
    for (int i=0; i< openWithActions.count(); i++)
        openWithMenu.addAction(&openWithActions[i]->action);
    QAction md5Act("MD5");
    QAction ed2kAct("ed2k");
    QAction renameAct("Rename...");
    QAction moveAct("Move...");
    QAction forpostAct("Forpost...");
    QAction showPreviewAct("Show preview");
    if (!file->isPartFile)
    {
        menu.addAction(&md5Act);
        menu.addAction(&ed2kAct);
        menu.addAction(&renameAct);
        menu.addAction(&moveAct);
        menu.addAction(&forpostAct);
        if (!file->forpost.isEmpty())
            menu.addAction(&showPreviewAct);
    }
    QMenu copyMenu("Copy");
    QAction copyFilePathAct("File path");
    copyMenu.addAction(&copyFilePathAct);
    QAction copyFileSizeAct("File size");
    copyMenu.addAction(&copyFileSizeAct);
    menu.addMenu(&copyMenu);
    menu.addSeparator();
    QAction deleteAct("Delete");
    if (!file->isPartFile)
        menu.addAction(&deleteAct);
    QAction * act = menu.exec(mapToGlobal(pos));
    if (act == &explorerAct)
        openWithExplorer(file);
    else if (act == & vlcAct)
        openWithVLC(file);
    else if (act == & mpcAct)
        openWithMPC(file);
    else if (act == & potAct)
        openWithPotPlayer(file);
    else if (act == & md5Act)
        calcMD5(file);
    else if (act == & ed2kAct)
        calcEd2k(file);
    else if (act == & forpostAct)
        forpost(file);
    else if (act == & deleteAct)
        deleteFile(row, fname);
    else if (act == & moveAct)
         mainWin()->moveFile(file);
    else if (act == & renameAct)
        renameMove(row, fname);
    else if (act == & copyFilePathAct)
        copyFilePath(file);
    else if (act == & copyFileSizeAct)
        copyFileSize(file);
    else if (act == & showPreviewAct)
        mainWin()->showPreview(file);
    else
        for (int i =0; i< openWithActions.count(); i++)
            if (act == &openWithActions[i]->action)
            {
                openWith(file, openWithActions[i]->program);
                break;
            }
}


void FilesView::headerPressed(int index)
{
    switch (index)
    {
    case 0:
        fmodel.sortByName(); break;
    case 1:
        fmodel.sortBySize(); break;
    case 2:
        fmodel.sortByDate(); break;
    case 3:
        fmodel.sortByMD5(); break;
    case 4:
        fmodel.sortByEd2k(); break;
    case 5:
        fmodel.sortByForpost(); break;
    case 6:
        fmodel.sortByDir(); break;
    default : break;
    }
}

void FilesView::filterSearchPressed()
{
    //qDebug() << "filterOkPressed";
    setFiles(&mainWin()->files, false);
    filterForm->searchPressed();
    if (!filterForm->calcFilter())
        return;
    fmodel.discardFilter();
    filterForm->filter.files.clear();
    filterForm->filter.execute(fmodel.files);
    fmodel.files = filterForm->filter.files;
    fmodel.refresh();
    mainWin()->setTabText(filterForm->stringForTab());
    qInfo() << "Files=" << fmodel.files.count();
}

void FilesView::searchBySize(qint64 sz)
{
    setFiles(&mainWin()->files, false);
    fmodel.discardFilter();
    filterForm->filter.files.clear();
    filterForm->setSizeFilter(sz);
    filterForm->filter.execute(fmodel.files);
    existingFiles = filterForm->filter.existingFiles;
    deletedFiles = filterForm->filter.deletedFiles;
    partFiles = filterForm->filter.partFiles;
    fmodel.files = filterForm->filter.files;
    fmodel.refresh();
    mainWin()->setTabText("<Size>");
}

void FilesView::searchByMD5(const QString &md5)
{
    setFiles(&mainWin()->files, false);
    fmodel.discardFilter();
    filterForm->filter.files.clear();
    filterForm->setMD5Filter(md5);
    filterForm->filter.execute(fmodel.files);
    fmodel.files = filterForm->filter.files;
    fmodel.refresh();
    mainWin()->setTabText("<MD5>");

}

void FilesView::filterAppendPressed()
{
    if (!filterForm->calcFilter())
        return;
//    model.discardFilter();
    filterForm->filter.files.clear();
    filterForm->filter.execute(fmodel.savedFiles);
    fmodel.appendFiles(filterForm->filter.files);
    fmodel.refresh();
    qInfo() << "Files=" << fmodel.files.count();
}

void FilesView::filterDiscardPressed()
{
    qDebug() << "filterDiscardPressed";
    setFiles(&mainWin()->files, false);
    fmodel.discardFilter();
}

void FilesView::bytesClicked()
{
    fmodel.showInBytes =  true;
    refresh();
}

void FilesView::mbytesClicked()
{
    fmodel.showInBytes =  false;
    refresh();
}

void FilesView::birthClicked()
{
    fmodel.showBirth = filterForm->birthCheckBox()->isChecked();
    refresh();
}

void FilesView::unsortPressed()
{
    fmodel.unSort();
}

void FilesView::onRefresh()
{
    refresh();
}

FilesModel::FilesModel(QVector<File *> *_files) : QAbstractTableModel(nullptr)
{
    files = *_files;
    showInBytes = false;
}

FilesModel::~FilesModel()
{
}

int FilesModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return files.count();
}

int FilesModel::columnCount(const QModelIndex & /*parent*/) const
{
    return 7;
}

void FilesModel::refresh()
{
    QModelIndex topLeft = index(0, 0);
       QModelIndex bottomRight = index(files.count(), 4);

       emit dataChanged(topLeft, bottomRight);
       emit layoutChanged();
}

QVariant FilesModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole)
    {
    if (orientation == Qt::Horizontal)
            switch(section)
            {
            case 0: return "Name";
            case 1: return "Size";
            case 2: return "Birth";
            case 3: return "MD5";
            case 4: return "ed2k";
            case 5: return "Forpost";
            case 6: return "Dir";
            default: return QAbstractTableModel:: headerData(section, orientation, role);
            }
    }
    return QAbstractTableModel:: headerData(section, orientation, role);
}

QVariant FilesModel::data(const QModelIndex &index, int role) const
{
    try{
    if (mainWin()->busy)
        return QVariant();
    if (!index.isValid())
        return QVariant();
    int r = index.row();
    if (r<0 || files.count() <=r) return QVariant();
    File* f = files[r];
    if (!f)  return QVariant();
    if (role == Qt::EditRole)
        qDebug() << "Edit";

    if (role == Qt::DisplayRole || role == Qt::EditRole)
    {
        QString s;
        switch(index.column())
        {
        case 0:
            s = files[r]->name; break;
        case 1:
            if (showInBytes)
            {
                QString s = QString("%L2").arg(files[r]->size);
                return s;
            }
            else
            {
                qreal mb = files[r]->size * 1.0 / (1024*1024);
                return mb;
            }
        case 2:
        {
            if (showBirth)
                return f->birthDate().toString("dd.MM.yy  hh:mm:ss");
            return f->date().toString("dd.MM.yy  hh:mm:ss");
        }
        case 3:
            return files[r]->MD5;
        case 4:
            return files[r]->ed2k;
        case 5:
            return files[r]->forpost;
        case 6:
            if (files[r]->info)
                s=files[r]->info->absolutePath();
            else if (files[r]->isPartFile)
                s = ((PartFile*)files[r])->partName;
            break;
        default: break;
        }
        return QVariant(s);
     }
     else if(role ==  Qt::ForegroundRole)
    {
          QColor col = Qt::black;
          if (f == activeFile)
              col = Qt::magenta;
          else if (f->isPartFile)
              col = Qt::darkGreen;
          else if (!f->exists)
              col = (Qt::gray);
          return col;
    }
    else if(role ==  Qt::BackgroundRole)
    {
        QColor col = Qt::white;
        if (/*r>0 && (filesAreEqual( f, files[r-1]) || */f->duplicate)
                col = QColor(255, 224, 224);
        return col;
    }


    return QVariant();
    }
    catch (...)
    {return QVariant();}
}

bool FilesModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    Q_UNUSED(role)
    if(index.column() != 0)
        return false;
    File* f = files[index.row()];
    if(f->info && !f->isPartFile)
    {
//        mutex.lock();
        QDir qdir = f->info->dir();
        QString newfn = value.toString();
        if (qdir.rename(f->name, newfn))
        {
            f->info->setFile(qdir, newfn);
            f->name = newfn;
            return true;
        }
        else qInfo() << "Rename failed.";
    }

    return true;
}

Qt::ItemFlags FilesModel::flags(const QModelIndex &index) const
{
    if (index.column() == 0)
        return Qt::ItemIsEditable | QAbstractTableModel::flags(index);
    else
        return QAbstractTableModel::flags(index);
}

void FilesModel::setFiles(QVector<File *> *_files, bool immed)
{
    savedFiles = *_files;
    if (immed)
        files = *_files;
}

void FilesModel::discardFilter()
{
    files =savedFiles;
    refresh();
}

void FilesModel::sortByName()
{
    if (sortOrder[0])
        std::sort(files.begin() , files.end(),
                  []( const File* file1 , const File* file2 )
            {
            return file1->name < file2->name;
        });
    else
        std::sort(files.begin() , files.end(),
                  []( const File* file1 , const File* file2 )
            { return file1->name > file2->name;});
    sortOrder[0] = !sortOrder[0];
    refresh();

}
void FilesModel::sortBySize()
{
    if (sortOrder[1])
        std::sort(files.begin() , files.end(),
                  []( const File* file1 , const File* file2 )
            { return file1->size < file2->size;});
    else
        std::sort(files.begin() , files.end(),
                  []( const File* file1 , const File* file2 )
            { return file1->size > file2->size;});
    sortOrder[1] = !sortOrder[1];
    refresh();
}

void FilesModel::sortByDate()
{
    if (sortOrder[2])
    {
        if (!showBirth)
        {
        std::sort(files.begin() , files.end(),
                  []( const File* file1 , const File* file2 )
            { return file1->date() < file2->date();});
        }
        else
        {
        std::sort(files.begin() , files.end(),
                  []( const File* file1 , const File* file2 )
            { return file1->birthDate() < file2->birthDate();});
        }
    }
    else
    {
        if (!showBirth)
        {
        std::sort(files.begin() , files.end(),
                  []( const File* file1 , const File* file2 )
            { return file1->date() > file2->date();});
        }
        else
        {
        std::sort(files.begin() , files.end(),
                  []( const File* file1 , const File* file2 )
            { return file1->birthDate() > file2->birthDate();});
        }
    }
    sortOrder[2] = !sortOrder[2];
    refresh();

}

void FilesModel::sortByMD5()
{
    if (sortOrder[3])
        std::sort(files.begin() , files.end(),
                  []( const File* file1 , const File* file2 )
            { return file1->MD5 < file2->MD5;});
    else
        std::sort(files.begin() , files.end(),
                  []( const File* file1 , const File* file2 )
            { return file1->MD5 > file2->MD5;});
    sortOrder[3] = !sortOrder[3];
    refresh();
}

void FilesModel::sortByEd2k()
{
    if (sortOrder[4])
        std::sort(files.begin() , files.end(),
                  []( const File* file1 , const File* file2 )
            { return file1->ed2k < file2->ed2k;});
    else
        std::sort(files.begin() , files.end(),
                  []( const File* file1 , const File* file2 )
            { return file1->ed2k > file2->ed2k;});
    sortOrder[4] = !sortOrder[4];
    refresh();

}

void FilesModel::sortByForpost()
{

    if (sortOrder[5])
        std::sort(files.begin() , files.end(),
                  []( const File* file1 , const File* file2 )
            { return file1->forpost < file2->forpost;});
    else
        std::sort(files.begin() , files.end(),
                  []( const File* file1 , const File* file2 )
            { return file1->forpost > file2->forpost;});
    sortOrder[5] = !sortOrder[5];
    refresh();

}

void FilesModel::sortByDir()
{
    if (sortOrder[3])
        std::sort(files.begin() , files.end(),
                  []( const File* file1 , const File* file2 )
            {
                QString s1 = file1->exists ? file1->info->absolutePath().toLower() : "";
                QString s2 = file2->exists ? file2->info->absolutePath().toLower() : "";
                return s1 < s2;
            });
    else
        std::sort(files.begin() , files.end(),
                  []( const File* file1 , const File* file2 )
            {
                QString s1 = file1->exists ? file1->info->absolutePath().toLower() : "";
                QString s2 = file2->exists ? file2->info->absolutePath().toLower() : "";
                return s1 > s2;
            });
    sortOrder[3] = !sortOrder[3];
    refresh();

}

void FilesModel::unSort()
{
    std::sort(files.begin() , files.end(),
              []( const File* file1 , const File* file2 )
        { return file1->id < file2->id;});
    for (int i=0; i< 4; i++)
        sortOrder[i] = false;
    refresh();
}

void FilesModel::appendFiles(const QVector<File *> &afiles)
{
    for (int i =0; i<afiles.count(); i++)
    {
        for (int j=0; j< files.count(); j++)
        {
            if (files[j] == afiles[i])
                goto nexti;
        }
            files.append(afiles[i]);
        nexti:;
    }
}

void FilesModel::showDuplicates()
{
    discardFilter();
    files.clear();
    for (int i=0; i< savedFiles.count(); i++)
        if (savedFiles[i]->duplicate)
            files.append(savedFiles[i]);
    refresh();
}


void FilesView::renameMove(int row, const QString &fname)
{
    QFileDialog fd;
    QFileInfo fi (fname);
    fd.setDirectory(fi.absolutePath());
    fd.selectFile(fi.fileName());
    if (fd.exec() == QDialog::Accepted && fd.selectedFiles().length() == 1 )
    {
        QFile f (fname);
        if (!f.exists())
            return;
        QString fs = fd.selectedFiles()[0];
        f.rename(fs);

        QFileInfo fi1 (fs);
        QString newName = fi1.fileName();
        fmodel.files[row]->name = newName;
//        QDir d (fs);
        QString newDir = fd.directory().path();
 /*       int nd = mainWin->dirInd(newDir);
        if (nd>=0)
        {
            model.files[row]->dirind = nd;
        }*/
        fmodel.refresh();

    }

}

void FilesView::forpost(File * _file)
{
    mainWin()->forpostDialog->setVideo(_file, _file->canonicalFilePath());
    mainWin()->forpostDialog->show();
    mainWin()->forpostDialog->raise();
    mainWin()->forpostDialog->setWindowState(Qt::WindowActive) ;
}

void FilesView::deleteFile(int row, const QString &fname)
{
    mainWin()->clearIgnoringFiles();
    mainWin()->addIgnoringFiles(QFileInfo(fname).fileName());
    auto btn =QMessageBox::question(mainWin(),"Delete file",
        QString("Do you really want to delete\n") + fname + "?",
        QMessageBox::Yes | QMessageBox::No);
    qDebug() << btn;
    if (btn == QMessageBox::Yes)
        while (true)
    {
 //       DeleteFileThread * df = new DeleteFileThread(fname);
 //       df->start();
        if (!QFile::remove(fname))
        {
            auto btn =QMessageBox::question(this,"Error",
                QString("File not deleted. Repeat?"),
                QMessageBox::Yes | QMessageBox::No);
//            qDebug() << btn;
            if (btn != QMessageBox::Yes)
                break;
        }
        else
        {
            fmodel.files[row]->exists = false;
            break;
        }
//        fmodel.refresh();
    }
    mainWin()->clearIgnoringFiles();
}

void FilesView::deleteDeleted(File *file)
{
    QString s = QString("delete from files where (fexists=0 and size=%1 and birth=%2)").
            arg(file->size).arg(file->birth.toMSecsSinceEpoch());
    QSqlQuery query(s);
//    qDebug() << query.lastError();
    commit();
//    qDebug() << query.lastError();
}

void FilesView::copyFilePath(File *file)
{
    QClipboard* c = QApplication::clipboard();
    c->setText(file->canonicalFilePath());

}
void FilesView::copyFileSize(File *file)
{
    //QClipboard* c = QApplication::clipboard();
    //c->setText(QString("%1").arg(file->size));
    filterForm->setSizeFilterEdit(QString("%1").arg(file->size));

}

bool FilesView::showBirth() const
{
    return filterForm->birthCheckBox()->isChecked();
}

void FilesView::setTabText(const QString &s)
{
    mainWin()->setTabText(s);
}

bool FilesView::showExisting()
{
    return filterForm->showExistingCheckBox()->isChecked();
}

bool FilesView::showDeleted()
{
    return filterForm->showExistingCheckBox()->isChecked();
}

void FilesView::activateFile(File *file)
{
    fmodel.discardFilter();
    int nr =-1;
    for (int i=0; i< fmodel.files.count(); i++)
        if (fmodel.files[i] == file)
        {
            nr = i;
            break;
        }
    fmodel.activeFile = file;
    if (nr >=0)
    {
        rowToSet = nr;
        QTimer::singleShot(500, this, SLOT(timerShot()));

    }
}

void FilesView::openWith(File *file, const QString &program)
{
    QStringList args;
    args << file->canonicalFilePath();
    openWithProcess.startDetached(program, args);
}

void FilesView::fillOpenWithActions()
{
    openWithActions.append(new OpenWith("Mediainfo", "e:/Program Files (x86)/K-Lite Codec Pack/Tools/mediainfo.exe"));
    openWithActions.append(new OpenWith("VirtualDub 11.05 64 bit", "e:/Downloads/VirtualDub/VirtDub1105r/VeeDub64.exe"));
    openWithActions.append(new OpenWith("VirtualDubMod", "e:/Downloads/VirtualDub/VirtDub1105r/VirtualDubMod.exe"));
    openWithActions.append(new OpenWith("VirtualDub2 64 bit", "e:/Downloads/VirtualDub/VirtualDub2_44282/VirtualDub64.exe"));
}

void FilesView::searchBySize(const QString &s)
{
    setFiles(&mainWin()->files, false);
    fmodel.discardFilter();
    filterForm->filter.files.clear();
    filterForm->setSizeFilterEdit(s);
    filterForm->filter.execute(fmodel.files);
    fmodel.files = filterForm->filter.files;
    fmodel.refresh();
    qInfo() << "Files=" << fmodel.files.count();

}

void FilesView::timerShot()
{
    if (rowToSet >=0)
    {
        QModelIndex modelIndex =  fmodel.index(rowToSet, 0, QModelIndex());
        qDebug() << "ActivateFile nr=" << rowToSet << modelIndex.row();
        scrollTo(modelIndex, QAbstractItemView::PositionAtTop);
        rowToSet = -1;

    }
}

FilesView::OpenWith::OpenWith(const QString &_menu, const QString &_program) : action (_menu), program(_program)
{
}

DeleteFileThread::DeleteFileThread(const QString &fn) : QThread(), fname(fn)
{
    connect(this, SIGNAL(finished()),this,SLOT(deleteLater()));
}

void DeleteFileThread::run()
{
    if (QFile::remove(fname))
        qDebug() << fname << "deleted";
    else
        qDebug() << fname << "not deleted";
}
