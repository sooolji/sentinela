#include "dbmanager.h"
#include <QDebug>

dbmanager::dbmanager(const QString &dbName, QObject* parent)
    : QObject(parent), m_currentUserId(-1)
{
    m_db = QSqlDatabase::addDatabase("QPSQL");
    m_db.setHostName("localhost");
    m_db.setDatabaseName(dbName);
    m_db.setUserName("postgres");
    m_db.setPassword(""); //

    if (!m_db.open()) {
        qDebug() << "Conexión a la base de datos falló:" << m_db.lastError().text();
    } else {
        qDebug() << "Conexión a la base de datos establecida con exitos";
    }
}

dbmanager::~dbmanager()
{
    if (m_db.isOpen()) {
        m_db.close();
    }
}

bool dbmanager::isOpen() const
{
    return m_db.isOpen();
}

QSqlError dbmanager::lastError() const
{
    return m_db.lastError();
}

QSqlDatabase& dbmanager::getDatabase()
{
    return m_db;
}

int dbmanager::currentUserId() const
{
    return m_currentUserId;
}

void dbmanager::setCurrentUserId(int id)
{
    m_currentUserId = id;
}
