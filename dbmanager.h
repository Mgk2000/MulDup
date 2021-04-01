#ifndef DBMANAGER_H
#define DBMANAGER_H

#include <QSqlDatabase>



class DbManager
{
public:
    DbManager();
    bool open();
    void close();
    void commit();
private:
    QSqlDatabase m_db;
};

#endif // DBMANAGER_H
