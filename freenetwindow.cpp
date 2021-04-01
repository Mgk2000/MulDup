#include "freenetwindow.h"
#include "ui_freenetwindow.h"
#include <QClipboard>
#include <QMimeData>
#include <QDesktopServices>
#include <QMouseEvent>
#include <QSqlQuery>
#include <QSqlError>
#include "hash.h"


FreenetWindow::FreenetWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::FreenetWindow)
{
    ui->setupUi(this);
    ui->videoView->setModel(new FreenetVideoModel);
    ui->videoView->setFont(QFont("Times",9));
    ui->videoView->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    ui->videoView->verticalHeader()->setDefaultSectionSize(9);
    //ui->videoView->verticalHeader()->setVisible(false);
    ui->videoView->horizontalHeader()->setStretchLastSection(true) ;
    ui->videoView->setColumnWidth(0, 300);
    ui->videoView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->videoView->viewport()->installEventFilter(this);
}

FreenetWindow::~FreenetWindow()
{
    delete ui;
}

void FreenetWindow::readClipboard()
{
    ((FreenetVideoModel*) ui->videoView->model())->readClipboard();
}

void FreenetWindow::refresh()
{
    ((FreenetVideoModel*) ui->videoView->model())->refresh();
}

bool FreenetWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == (QObject*) ui->videoView->viewport())
    {
        QMouseEvent * mev = dynamic_cast<QMouseEvent *>( event);
        if (mev && mev->type() == QMouseEvent::MouseButtonRelease && mev->button() ==  Qt::RightButton)
        {
            mouseEvent(mev);
            return true;
        }
    }
    return QObject::eventFilter(obj, event);;
}

void FreenetWindow::mouseEvent(QMouseEvent *event)
{
    qDebug() << event;
    QPoint pos = event->pos();
    QPersistentModelIndex index = ui->videoView->indexAt(pos);
    int currRow = index.row();
    qDebug() << index.row() << index.column();
    if (currRow <0 || index.column() <0)
        return;
//    qInfo() << model.files[index.row()]->name;
    if(event->button() == Qt::RightButton)
    {
        if(index.row() >=0)
            contextMenu(index.row(), pos);
        //emit activated(index);
    }
/*    else
        ui->videoView->mousePressEvent(event);*/

}

void FreenetWindow::contextMenu(int row, const QPoint &pos)
{
    FreenetVideoModel* mod =(FreenetVideoModel*) ui->videoView->model();
    QModelIndex index = mod->index(row, 0);
//    Then you can set that modelindex as the current one using
    ui->videoView->setCurrentIndex(index);
    QMenu menu;
    QAction copyVidAct("Copy video key");
    menu.addAction(&copyVidAct);
    QAction copyVidNameAct("Copy video name");
    menu.addAction(&copyVidNameAct);
    menu.addSeparator();
    QAction copyPicAct("Copy picture");
    menu.addAction(&copyPicAct);
    QAction openPicAct("Open picture");
    menu.addAction(&openPicAct);
    QAction* act = menu.exec(mapToGlobal(pos));
    FreenetVideo& vid = mod->videos[row];
    if (act == &copyVidAct)
        QApplication::clipboard()->setText(vid.videoKey +'/' + vid.videoName );
    else if (act == &copyVidNameAct)
        QApplication::clipboard()->setText(stripExtension( vid.videoName) );
    else if (act == &copyPicAct)
        QApplication::clipboard()->setText(vid.picKey +'/' + vid.picName );

    else if (act == &openPicAct)
    {
        QString url= "http://127.0.0.1:8888/freenet:" + vid.picKey+"/" + vid.picName;
        QDesktopServices::openUrl ( url );
        model()->videos[row].stored = true;
        model()->storeVideos(row, row);
    }
}

void FreenetVideoModel::storeVideos(int beg, int end)
{
    QSqlQuery query;
    for (int i=beg; i<=end; i++)
    {
        FreenetVideo& vid = videos[i];
        QString s;
        if (!vid.picName.isEmpty())
        s = QString("insert or ignore into freenet_videos (fname, key, picname, pickey) values (\"%1\", \"%2\", \"%3\", \"%4\")")
             .arg(vid.videoName).arg(vid.videoKey).arg(vid.picName).arg(vid.picKey);
        else
            s = QString("insert or ignore into freenet_videos (fname, key, picname, pickey) values (\"%1\", \"%2\")")
                 .arg(vid.videoName).arg(vid.videoKey);
        if (!query.exec(s))
        {
            qDebug() << s;
            qDebug() << "insert error:"
                            << query.lastError();
        }
        vid.stored= true;
    }
    refresh();
}

FreenetVideoModel *FreenetWindow::model()
{
    return (FreenetVideoModel *) ui->videoView->model();
}

int FreenetVideoModel::rowCount(const QModelIndex & /*parent*/) const
{
    return videos.count();;
}

void FreenetVideoModel::refresh()
{
    QModelIndex topLeft = index(0, 0);
       QModelIndex bottomRight = index(videos.count(), 2);

       emit dataChanged(topLeft, bottomRight);
       emit layoutChanged();

}

void FreenetVideoModel::openInBrowser()
{
    if (openInd >= videos.count()-1)
        openInd =-1;
    int i =0;
    qDebug() << "--------------------------------------------------";
    int beg = openInd+1;
    for (; ;)
    {
        openInd++;
        if (videos[openInd].picName != "" && !videos[openInd].stored)
        {
            QString url= "http://127.0.0.1:8888/freenet:" + videos[openInd].picKey+"/" + videos[openInd].picName;
            qDebug() << "Open " << openInd << videos[openInd].picName;
            QDesktopServices::openUrl ( url );
            videos[openInd].stored = true;
            i++;
            if (i==30)
                goto store;
        }
        if (openInd>= videos.count()-1)
            break;
    }
    store:
    storeVideos(beg, openInd);
}

void FreenetVideoModel::readClipboard()
{
    if( QClipboard* c = QApplication::clipboard() )
        if( const QMimeData* m = c->mimeData() )
            if( m->hasText() )
            {
                videos.clear();
                pictures.clear();
                openInd =-1;
                QString s = m->text();
                int beg =0;
                for(;;)
                {
                    int end = s.indexOf('\n', beg);
                    if (end<0)
                         end = s.length();
                    QString ss = stripHttp(s.mid(beg, end-beg));
                    if (isFreenetKey(ss))
                    {
                        if (isVideo(ss))
                        {
                            FreenetVideo video;
                            video.videoName = stripDirectory(ss);
                            video.videoKey = freenetKey(ss);
                            videos.append(video);
                        }
                        else if (isPicture(ss))
                        {
                            pictures.append(ss);
                        }
                     }
                    beg = end+1;
                    if (beg >= s.length() - 10)
                        break;
                    for (int i = beg; i<s.length(); i++)
                        if (s[i] == '\r' || s[i] == '\n')
                            beg++;
                        else break;

                }
                for (int i =0; i< pictures.count(); i++)
                {
                    QString pn = stripDirectory(pictures[i]);
                    for (int j=0; j< videos.count(); j++)
                            if (stripExtension(stripExtension(pn)).contains(
                                    stripExtension(videos[j].videoName)))
                            {
                                videos[j].picName = pn;
                                videos[j].picKey = freenetKey(pictures[i]);
                            }
                }
            }
    calcStored();
    refresh();
}

void FreenetVideoModel::calcStored()
{
    for (int i=0; i< videos.count(); i++)
        checkStored(videos[i]);
}

void FreenetVideoModel::checkStored(FreenetVideo &fv)
{
    QString s(QString("select key from freenet_videos where key=\"%1\"").arg(fv.videoKey));
//    QString s(QString("select key from freenet_videos where fname=\"%1\"").arg(fv.videoName));
    QSqlQuery query;
    auto rc = query.exec(s);
    if (!rc)
        qDebug() << "Select error:"
                        << query.lastError();

//    int sz = query.size();
    fv.stored = query.next();
}

QVariant FreenetVideoModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole)
    {
    if (orientation == Qt::Horizontal)
            switch(section)
            {
            case 0: return "Video";
            case 1: return "Preview";
            default: return QAbstractTableModel:: headerData(section, orientation, role);
            }
    }
    return QAbstractTableModel:: headerData(section, orientation, role);
}

QVariant FreenetVideoModel::data(const QModelIndex &index, int role) const
{
    int row = index.row();
    int col = index.column();
    if (row <0)
        return QVariant();
    const FreenetVideo& vid = videos[row];
    if (role == Qt::DisplayRole)
    {
        if (col ==0)
            return vid.videoName;
        else
            return vid.picName;
    }
    else if(role ==  Qt::BackgroundRole)
    {
        QColor col = vid.stored ? QColor(255, 224, 224) : Qt::white;
        return col;
    }

    return QVariant();
}

void FreenetVideoModel::readStoredVideos()
{
    storedVideos.clear();
    QSqlQuery query("select name, key from freenet_videos");
    while (query.next())
    {
        StoredVideo sv;
        sv.fmame = query.value(0).toString();
        sv.key = query.value(1).toString();
        storedVideos.append(sv);
    }
}

void FreenetWindow::on_actionRead_clipboard_triggered()
{
    readClipboard();
}

void FreenetWindow::on_actionBrowse_triggered()
{
    ((FreenetVideoModel*) ui->videoView->model())->openInBrowser();
}

void FreenetWindow::on_actionStore_all_triggered()
{
    if(model()->videos.length() >0)
        model()->storeVideos(0, model()->videos.length()-1);

}
