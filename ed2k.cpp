#include "ed2k.h"
#include <QDebug>
#include <QFile>
#include <QByteArray>
#include "hash.h"

PartMetFile::PartMetFile(const QString &fn): name(""), size(0)
{
    if (read(partDir() + "/" + fn + ".met"))
         partName = fn;
    else partName = "";
}
#define RET {  return;}
bool PartMetFile::read(const QString &fn)
{
    qDebug() << fn;
    size = -1;
    transferred = -1;
    name = "";
    QFile f (fn);
    if (!f.open(QIODevice::ReadOnly))
        return false;
    char  zbuf [300000];
     if (f.read(zbuf, f.size()) < 0)
         return false;
    char* ptr = zbuf;
    uchar version = (uchar) zbuf[0];
    bool isnewstyle = (version == PARTFILE_SPLITTEDVERSION);
    if (isnewstyle)
        return false;
    int utc = *((int*) & zbuf[1]);
    dateTime.setSecsSinceEpoch(utc);
    qDebug() << "UTC=" << dateTime;
    QByteArray ba (&zbuf[5], 16);
    hash = ba.toHex();
    int nhashes = *((short*) & zbuf[21]);
    ptr += 23 + nhashes* 16;
    int ntags = *((int*) ptr);
    ptr+=4;
    char tagtype, tagname;
    int taglen,slen;
    for (int i=0; i< ntags; i++)
    {
        if (size>0 && transferred>0 && name != "")
            return true;
        tagtype = *ptr;
        ptr++;
        taglen = *((short*) ptr);
        if (taglen !=1)
        {
            qDebug() << "Error taglen=" << taglen << " tagname=" << (int)tagname << " tagtype=" << (int)tagtype;
            return true;
        }
        ptr+=2;
        tagname = *ptr;
        ptr++;
        switch (tagname)
        {
        case FT_FILENAME:
            {
                slen = *((short*) ptr);
                ptr +=2;
                QByteArray ba (ptr, slen);
                name = ba;
                ptr+=slen;
                break;
            }
        case FT_FILESIZE:
        case FT_TRANSFERRED:
            {
                qint64 sz;
                if (tagtype == TAGTYPE_UINT32)
                {
                    sz = *((uint*) ptr);
                    ptr+=4;
                }
                else if (tagtype == TAGTYPE_UINT64)
                {
                    sz = *((quint64*) ptr);
                    ptr+=8;
                }
                else
                {
                    qDebug() << "Size Error TagType" << tagtype;
                    return true;
                }
                switch(tagname)
                {
                case FT_TRANSFERRED: transferred = sz; break;
                case FT_FILESIZE:    size = sz; break;
                }

                break;

            }
         default:
            switch(tagtype)
            {
            case TAGTYPE_UINT8:
                    ptr++; break;
            case TAGTYPE_UINT16:
                    ptr+=2; break;
            case TAGTYPE_UINT32:
            case TAGTYPE_FLOAT32:
                ptr+=4; break;
            case TAGTYPE_UINT64:
                    ptr+=8; break;
            case TAGTYPE_STRING:
                    slen = *((short*) ptr);
                    ptr +=2 + slen; break;

            default:
                qDebug()<< "Unknown tag type =" << (int) tagtype;
                return true;
            }

        }
    }
    return true;
}


PartFile::PartFile(PartMetFile *pmf)
{
    this->name = pmf->name;
    this->ed2k = pmf->hash;
    this->MD5 = "";
    this->birth = pmf->dateTime;
    this->exists = false;
    this->size = pmf->size;
    this->info = nullptr;
    isPartFile = true;
    partName =pmf->partName;
}

QString PartFile::canonicalFilePath() const
{
    return partDir() + "\\" + partName;
}
