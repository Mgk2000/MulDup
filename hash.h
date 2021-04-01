#ifndef HASH_H
#define HASH_H
#include <QCryptographicHash>
#include <QString>
#include <QDebug>
#include "file.h"
QString getHash(const QString& fname, QCryptographicHash::Algorithm alg);
QString getEd2k(const QString& fname);

QString quickHash(const QString& fname);
void openWithVLC(File* f);
void openWithExplorer(File* f);
void openWithMPC(File* f);
void openWithKMPlayer(File* f);
inline QString partDir(){return "u:\\Downloads\\eMule0.51d\\t";}
void calcMD5(File * file);
void calcEd2k(File * file);
void commit();
bool filePathsAreEqual(const QString& s1, const QString & s2);
class MainWindow;
void setMainWin(MainWindow* mw);
MainWindow* mainWin();
void msgHandler(QtMsgType type,
          const QMessageLogContext &context, const QString &msg);
void Log(const QString& s);
void fillvideoExts();
bool isFreenetKey(const QString &s);
QString stripDirectory(const QString & fn);
QString stripExtension(const QString & fn);
QString stripHttp(const QString & fn);
QString freenetKey(const QString & fn);


bool isVideo(const QString & fname);
bool isPicture(const QString &fn);
QStringList & videoFilters();

QString fileExt(const QString & fn);

#endif // HASH_H
