#ifndef AMENAZA_H
#define AMENAZA_H

#include <QWidget>
#include <QSqlRecord>
#include <QSqlTableModel>
#include <QShowEvent>
#include <QTextDocument>
#include <QFileDialog>
#include "dialogs/exportdialog.h"
#include "database/dbmanager.h"

namespace Ui {
class amenaza;
}

class amenaza : public QWidget
{
    Q_OBJECT

public:
    explicit amenaza(QWidget *parent = nullptr);
    ~amenaza();

protected:
    void showEvent(QShowEvent *event) override;

private slots:
    void on_pushButton_clicked();
    void on_pushButton_2_clicked();
    void on_pushButton_3_clicked();
    void on_pushButton_4_clicked();
    void on_pushButton_5_clicked();

private:
    Ui::amenaza *ui;
    dbmanager *m_dbmanager;

    void setupTableModel();
    void connectButtons();

    void exportToCSV(const QString &filePath);
    void exportToJSON(const QString &filePath);
};

#endif // AMENAZA_H
