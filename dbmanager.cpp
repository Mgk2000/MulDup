#include "dbmanager.h"
#include <QDebug>
#include <QSqlQuery>

DbManager::DbManager()
{
    m_db = QSqlDatabase::addDatabase("QSQLITE");
    m_db.setDatabaseName("u:/MulDup/Db/muldup.db");
}

bool DbManager::open()
{
    if (!m_db.open())
    {
       qDebug() << "Error: connection with database failed";
       return false;
    }
    else
    {
       qDebug() << "Database: connection ok";
       m_db.exec("PRAGMA synchronous = OFF");
       m_db.exec("PRAGMA journal_mode = MEMORY");
       return true;
    }

}

void DbManager::close()
{
    m_db.close();
}

void DbManager::commit()
{
    m_db.commit();
}
