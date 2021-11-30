#include "file.h"
#include "dir.h"

#include <QDir>


File::File(const QString &fullFn): duplicate(false)
{
    info = new QFileInfo;
    setFileName(fullFn);
}

File::~File()
{
    if (info)
        delete info;
}

QDateTime File::date() const
{
    if (info)
    {
        QDateTime dt = info->lastModified();
        if (dt>birth || true)
            return dt;
        else return birth;
    }
    return birth;
}

QDateTime File::birthDate() const
{
    if (info)
    {
        QDateTime dt = info->birthTime();
        return dt;
    }
    return birth;

}

void File::setFileName(const QString &fullfn)
{
    info->setFile(fullfn);
    if (info->exists())
    {
        name = info->fileName();
        size = info->size();
        birth = std::max(info->birthTime(), info->lastModified());
        exists = true;
    }
    else
        exists = false;
    isPartFile = false;

}

QString File::canonicalFilePath() const
{
    return QDir::toNativeSeparators(info->canonicalFilePath());
}

QString File::filePath() const
{
    if (info)
        return info->filePath();
    else
        return "";
}

bool filesAreEqual(File *f1, File *f2)
{
    if (!f1->exists || !f2->exists)
        return false;
    if (f1->MD5 == "" || f2->MD5 == "")
        return f1->size == f2->size;
    else return f1->MD5 == f2->MD5;
}
