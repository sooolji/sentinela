#ifndef DBMANAGER_H
#define DBMANAGER_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlError>

class dbmanager : public QObject
{
    Q_OBJECT
public:
    static dbmanager& instance(const QString &dbName = "sentinela") {
        static dbmanager instance(dbName);
        return instance;
    }

    bool isOpen() const;
    QSqlError lastError() const;
    QSqlDatabase& getDatabase();
    int currentUserId() const;
    void setCurrentUserId(int id);

private:
    explicit dbmanager(const QString &dbName, QObject* parent = nullptr);
    ~dbmanager();

    QSqlDatabase m_db;
    int m_currentUserId;
};

#endif // DBMANAGER_H
