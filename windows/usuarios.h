#ifndef USUARIOS_H
#define USUARIOS_H

#include <QWidget>
#include <QSqlTableModel>
#include <QTableView>
#include <QRect>
#include <QJsonObject>
#include "database/dbmanager.h"

namespace Ui {
class usuarios;
}

class usuarios : public QWidget
{
    Q_OBJECT

public:
    explicit usuarios(QWidget *parent = nullptr);
    void hideTechnicalColumns();
    ~usuarios();

private slots:
    void on_pushButton_clicked();
    void on_pushButton_2_clicked();
    void on_pushButton_3_clicked();
    void on_pushButton_4_clicked();
    void on_pushButton_5_clicked();
    void refreshTable();

private:
    void setupTableModel();
    bool exportToCsv(const QString &fileName);
    bool exportToJson(const QString &fileName);
    void logAction(const QString& action, int record_id, const QString& table,
                   const QJsonObject& previous = QJsonObject(),
                   const QJsonObject& current = QJsonObject());

    Ui::usuarios *ui;
    dbmanager *m_dbmanager;
    QSqlTableModel *model;
};

#endif // USUARIOS_H
