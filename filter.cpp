#include "filter.h"
#include "file.h"
Filter::Filter(): existingFiles(0), deletedFiles(0), partFiles(0)
{

}

void Filter::addAtom(FilterAtom * a)
{
    filterAtoms.append(a);
}

void Filter::execute(QVector<File *>& _files)
{
    existingFiles = 0;
    deletedFiles = 0;
    partFiles = 0;
    for (int i=0; i< _files.count(); i++)
    {
        for (int j=0; j< this->filterAtoms.count(); j++)
            if (!filterAtoms[j]->correct(*(_files[i])))
                goto nextfile;
        files.append(_files[i]);
        if (_files[i]->exists)
        {
            if (_files[i]->isPartFile)
                partFiles ++;
            else
                existingFiles ++;
        }
        else
            deletedFiles++;
        nextfile:;
    }
}

bool FilterSizeMore::correct(const File &f)
{
    return f.size >= size;
}

bool FilterSizeLess::correct(const File &f)
{
    return f.size <= size;
}

bool FilterSizeEq::correct(const File &f)
{
    return f.size == size;
}

bool FilterNameContains::correct(const File &f)
{
    return f.name.toLower().contains(pattern);
}

bool FilterExisting::correct(const File &f)
{
    return (!f.exists || f.isPartFile);
}

bool FilterDeleted::correct(const File &f)
{
    return (f.exists || f.isPartFile);
}
bool FilterPart::correct(const File &f)
{
    return  !f.isPartFile;
}

bool FilterMD5::correct(const File &f)
{
    return f.MD5.toLower() == md5.toLower();
}
