#include "filter.h"
#include "file.h"
Filter::Filter()
{

}

void Filter::addAtom(FilterAtom * a)
{
    filterAtoms.append(a);
}

void Filter::execute(QVector<File *>& _files)
{
    for (int i=0; i< _files.count(); i++)
    {
        for (int j=0; j< this->filterAtoms.count(); j++)
            if (!filterAtoms[j]->correct(*(_files[i])))
                goto nextfile;
        files.append(_files[i]);
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
