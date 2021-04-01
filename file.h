#ifndef FILE_H
#define FILE_H

#include <QString>
#include <QFileInfo>
#include <QDateTime>
class Dir;

class File
{
public:
    File(QFileInfo* _info): info(_info) , isPartFile(false), duplicate(false) {}
    File () : info(nullptr), isPartFile(false), duplicate(false){}
    File(const QString& fullFn);
    virtual ~File();
    int id;
    QString name;
    QFileInfo* info;
    quint64 size;
    QDateTime birth;
    QDateTime date() const;
    QString MD5, ed2k, forpost;
    bool exists;
    bool isPartFile;
    void setFileName(const QString& fullfn);
    virtual QString canonicalFilePath() const;
    virtual QString filePath() const;
    bool duplicate;
};
bool filesAreEqual(File* f1, File* f2);

#endif // FILE_H
