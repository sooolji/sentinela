#ifndef ACTIVOS_H
#define ACTIVOS_H

#include <QWidget>
#include <QSqlDatabase>
#include <QSqlRecord>
#include <QSqlTableModel>
#include <QShowEvent>
#include <QTextDocument>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

class dbmanager;

namespace Ui {
class activos;
}

class activos : public QWidget
{
    Q_OBJECT

public:
    explicit activos(QWidget *parent = nullptr, dbmanager* dbManager = nullptr);
    ~activos();

private slots:
    void on_pushButton_clicked(); // Search
    void on_pushButton_2_clicked(); // Add
    void on_pushButton_3_clicked(); // Edit
    void on_pushButton_5_clicked(); //Delete
    void on_pushButton_6_clicked();// Export

protected:
    void showEvent(QShowEvent *event) override;

private:
    Ui::activos *ui;
    dbmanager* m_dbManager;

    void setupTableModel();
    void searchAssets(const QString& searchText);
    void addNewAsset();

    void exportToCSV();
    void exportToJSON();
    QString getCurrentEntityName() const;
    QPixmap getEntityLogo() const;
};

#endif // ACTIVOS_H
