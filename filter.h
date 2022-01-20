#ifndef FILTER_H
#define FILTER_H
#include <QFileInfo>
class File;
struct FilterAtom
{
    virtual ~FilterAtom() {}
    virtual bool correct (const File& f) =0;
};
struct FilterSizeMore : public FilterAtom
{
    FilterSizeMore(quint64 sz) : size(sz) {}
    virtual ~FilterSizeMore() {}
    bool correct (const File& f) override;
    quint64 size;
};
struct FilterSizeLess : public FilterAtom
{
    FilterSizeLess(quint64 sz) : size(sz) {}
    virtual ~FilterSizeLess() {}
    bool correct (const File& f) override;
    quint64 size;
};

struct FilterSizeEq : public FilterAtom
{
    FilterSizeEq(quint64 sz) : size(sz) {}
    virtual ~FilterSizeEq() {}
    bool correct (const File& f) override;
    quint64 size;
};

struct FilterNameContains : public FilterAtom
{
    FilterNameContains(const QString& s) : pattern(s.toLower()) {}
    virtual ~FilterNameContains() {}
    bool correct (const File& f) override;
    QString  pattern;
};
struct FilterExisting : public FilterAtom
{
    FilterExisting(){}
    virtual ~FilterExisting() {}
    bool correct (const File& f) override;
};
struct FilterDeleted : public FilterAtom
{
    FilterDeleted() {}
    virtual ~FilterDeleted() {}
    bool correct (const File& f) override;
};
struct FilterPart : public FilterAtom
{
    FilterPart() {}
    virtual ~FilterPart() {}
    bool correct (const File& f) override;
};
struct FilterMD5 : public FilterAtom
{
    FilterMD5(const QString & _md5) : md5(_md5) {}
    virtual ~FilterMD5() {}
    bool correct (const File& f) override;
    QString md5;
};

class Filter
{
public:
    Filter();
    QList<FilterAtom*> filterAtoms;
    void addAtom(FilterAtom*);
    void execute(QVector<File*>& _files);
    QVector<File*> files;
};

#endif // FILTER_H
