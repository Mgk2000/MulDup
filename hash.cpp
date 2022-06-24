#include "hash.h"
#include "ed2k.h"
#include "mainwindow.h"
#include <QFile>
#include <QFileInfo>
#include <QDebug>
#include <QDateTime>
#include <QProcess>
#include <QDir>
#include <QSqlQuery>
#include <QSqlError>
MainWindow* mainWindow;
void setMainWin(MainWindow* mw)
{
    mainWindow = mw;
}
MainWindow* mainWin()
{
    return mainWindow;
}
QString getHash(const QString& fname, QCryptographicHash::Algorithm alg)
{
    if (fileIsTooNew(fname))
        return "";

    QCryptographicHash crypto(alg);
    QFile file(fname);
    if (!file.open(QFile::ReadOnly))
        return "";
    char buf[8192];
    while(!file.atEnd())
    {
      int nbytes = file.read(buf, 8192);
      if (nbytes<0)
          return "";
      crypto.addData(buf, nbytes);
    }
    QByteArray hash = crypto.result();
    return hash.toHex();
}
QString quickHash(const QString &fname)
{
    QFileInfo fi (fname);
    return QString("%1%2").arg(fi.size()).arg(fi.birthTime().toString() /*currentSecsSinceEpoch()*/);
}

void calcMD5(File *  file)
{
//    if (file->MD5 != "")
//        return;
    file->MD5 = getHash(file->canonicalFilePath(), QCryptographicHash::Md5);
//    QString shash = quickHash(fname);
    qDebug() << file->MD5;
//    quint64 msec = file->birth.toMSecsSinceEpoch();
    QString sq(QString("update files set MD5=\"%1\" where (id = %2)").
            arg(file->MD5).arg(file->id));
    QSqlQuery query(sq);
    bool  err = query.lastError().isValid();
    if ( err)
        qDebug() << "set MD5 error:"
                        << query.lastError();
//    else commit();

}
void openWithVLC(File* f)
{
    QString program = "e:\\Program Files\\VideoLAN\\VLC\\vlc.exe";
    QStringList arguments;
    arguments << f->canonicalFilePath();
    QProcess::startDetached(program, arguments);
}
void openWithExplorer(File* f)
{
        QString program = "explorer.exe";
        QStringList arguments;
        arguments << QString("/select,");
        arguments << QString("/n,");
        arguments << f->canonicalFilePath();
        QProcess::startDetached(program, arguments);
}


void openWithMPC(File* f)
{
    QString program = "e:/Program Files (x86)/K-Lite Codec Pack/MPC-HC64/mpc-hc64.exe";
    QStringList arguments;
    arguments << f->canonicalFilePath();
    QProcess::startDetached(program, arguments);
}

void openWithPotPlayer(File* f)
{
    QString program = "C:/Program Files/DAUM/PotPlayer/PotPlayerMini64.exe";
    QStringList arguments;
    arguments << f->canonicalFilePath();
    QProcess::startDetached(program, arguments);
}
void msgHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QByteArray localMsg = msg.toLocal8Bit();
    QString s;
    char buf[1000];
    const char *file = context.file ? context.file : "";
    const char *function = context.function ? context.function : "";
    switch (type) {
    case QtDebugMsg:
//        sprintf(buf, "Debug: %s (%s:%u, %s)\n", localMsg.constData(), file, context.line, function);
//        fprintf(stderr, "Debug: %s (%s:%u, %s)\n", localMsg.constData(), file, context.line, function);
//        sprintf(buf, "%s\n", localMsg.constData());
        fprintf(stderr, "%s\n", localMsg.constData());
        fflush(stderr);
        break;
    case QtInfoMsg:
//        sprintf(buf, "Info: %s (%s:%u, %s)\n", localMsg.constData(), file, context.line, function);
        sprintf(buf, "%s\n", localMsg.constData());
        fprintf(stderr, "Info: %s (%s:%u, %s)\n", localMsg.constData(), file, context.line, function);
//        mainWin()->logList << QString(buf);
    {
        QString s = QString(buf);
        s.truncate(s.length()-1);
        mainWin()->Log(s);
    }

        break;
    case QtWarningMsg:
//        sprintf(buf, "Warning: %s (%s:%u, %s)\n", localMsg.constData(), file, context.line, function);
        fprintf(stderr, "Warning: %s (%s:%u, %s)\n", localMsg.constData(), file, context.line, function);
        break;
    case QtCriticalMsg:
        sprintf(buf, "Critical: %s (%s:%u, %s)\n", localMsg.constData(), file, context.line, function);
        fprintf(stderr, "Critical: %s (%s:%u, %s)\n", localMsg.constData(), file, context.line, function);
        break;
    case QtFatalMsg:
//        sprintf(buf, "Fatal: %s (%s:%u, %s)\n", localMsg.constData(), file, context.line, function);
        fprintf(stderr, "Fatal: %s (%s:%u, %s)\n", localMsg.constData(), file, context.line, function);
        break;
    }
//    if (sdebug->bEmit)
//        emit sdebug->stringChanged(sdebug->slist);
}


void Log(const QString &s)
{
    mainWin()->Log(s);
}

void commit()
{
    mainWin()->commit();
}
bool fileIsTooNew(const QString& fname)
{
    QFileInfo fi(fname);
    qint64 dt = QDateTime::currentMSecsSinceEpoch() -
            fi.fileTime(QFileDevice::FileModificationTime).toMSecsSinceEpoch();
    qDebug() << "dt=" << dt;
    if (dt <5000 && dt >=0)
    {
        qDebug() << fname << "is too new";
        return true;
    }
    else

    {
        qDebug() << fname << "is ok old";
        return false;
    }
}
QString getEd2k(const QString &fname)
{
    QCryptographicHash crypto(QCryptographicHash::Md4);
    if (fileIsTooNew(fname))
        return "";
    QFile file (fname);
    if (!file.open(QIODevice::ReadOnly))
        return "";
    qint64 sz = file.size();
    qint64 blockSize = 9500 * 1024;
    int nblocks = sz / blockSize;
    int tail = sz - blockSize * nblocks;
    if (tail !=0) nblocks++;
    char * buf = new char [9500 * 1024];
    int nbytes = file.read(buf, blockSize);
    if (nbytes < 0)
    {
        delete[] buf;
        return "";
    }
    crypto.addData(buf, nbytes);
    QByteArray hash = crypto.result();
    if (nblocks ==1)
        return  hash.toHex();
    int nb=0;
    while (!file.atEnd())
    {
        nbytes = file.read(buf, blockSize);
//        qDebug() << "nb=" << nb++  << "pos=" << file.pos() << "nbytes=" << nbytes;
        crypto.reset();
        crypto.addData(buf, nbytes);
        hash += crypto.result();
    }
    crypto.reset();
    crypto.addData(hash);
    hash = crypto.result();
    delete[] buf;
    return  hash.toHex();
}

bool filePathsAreEqual(const QString &s1, const QString &s2)
{
    if (s1 == s2)
        return true;
    QString ss1 = s1.toLower();
    ss1.replace('\\', '/');
    QString ss2 = s2.toLower();
    ss2.replace('\\', '/');
    return ss1 == ss2;
}
QStringList videoExts, pictureExts;
QStringList vidFilters;

void fillvideoExts()
{
    videoExts.clear();
    videoExts << "mpg" << "avi" << "mp4" << "wmv" << "mkv" << "flv" << "mov" << "vob" << "m4v"  << "m1v" << "ogg"
    << "3gp" << "asf" << "mpeg" << "divx" << "webm" << "rm"  << "rmvb" << "qt"<< "ts" << "mts" << "m2t"
    << "ogm";
    vidFilters.clear();
    for (int i =0; i< videoExts.count(); i++)
        vidFilters << "*." + videoExts[i];
    pictureExts.clear();
    pictureExts << "jpg" << "jpeg" << "gif" << "png";
}
QStringList & videoFilters()
{
    return vidFilters;
}

bool isVideo(const QString &fn)
{
    return videoExts.contains(fileExt(fn).toLower());
}
bool isPicture(const QString &fn)
{
    return pictureExts.contains(fileExt(fn).toLower());
}

QString fileExt(const QString &fn)
{
    int np = fn.lastIndexOf('.');
    if (np<=0)
        return "";
    return fn.mid(np+1, -1);
}
bool isFreenetKey(const QString &s)
{
    QString s1 = s.mid(0,4);
    return (s1 =="CHK@" || s1 == "SSK@" );
}

QString stripDirectory(const QString &fn)
{
    int i = fn.lastIndexOf('/');
    return fn.mid(i+1);
}

QString stripExtension(const QString &fn)
{
    int i = fn.lastIndexOf('.');
    return fn.mid(0, i);
}

QString freenetKey(const QString &fn)
{
    int i = fn.lastIndexOf('/');
    return fn.mid(0,i);
}

QString stripHttp(const QString &fn)
{
    if (fn.mid(0,5)=="http:")
        return fn.mid(22);
    else return fn;
}

void calcEd2k(File *file)
{
    file->ed2k = getEd2k(file->filePath());
}

bool isMulDir(const QString &sdir)
{
    return sdir.toLower() == "u:/y/muld";
}
