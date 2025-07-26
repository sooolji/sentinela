#ifndef IMPACTO_H
#define IMPACTO_H

#include <QWidget>
#include "database/dbmanager.h"

namespace Ui {
class impacto;
}

class impacto : public QWidget
{
    Q_OBJECT

public:
    explicit impacto(QWidget *parent = nullptr);
    ~impacto();

protected:
    void showEvent(QShowEvent *event) override;

private slots:
    void onSearchClicked();
    void onAddClicked();
    void onEditClicked();
    void onDeleteClicked();
    void onExportClicked();

private:
    Ui::impacto *ui;
    dbmanager* m_dbManager;

    void exportToCSV(const QString &fileName);
    void exportToJSON(const QString &fileName);

    void setupTableModel();
    void connectButtons();
};

#endif // IMPACTO_H
