#ifndef AUDITORIA_H
#define AUDITORIA_H

#include <QWidget>
#include <QMenu>
#include "dialogs/exportdialog.h"

namespace Ui {
class auditoria;
}

class auditoria : public QWidget
{
    Q_OBJECT

public:
    explicit auditoria(QWidget *parent = nullptr, int userId = -1);
    ~auditoria();

protected:
    void showEvent(QShowEvent *event) override;

private slots:
    void onBuscarClicked();
    void onFiltrarTextoCambiado(const QString &text);
    void onExportarClicked();
    void exportarCSV(const QString &fileName);
    void exportarJSON(const QString &fileName);

private:
    Ui::auditoria *ui;
    int m_userId;
    QString getFileFilters(exportdialog::ExportFormat format) const;
    void setupTableModel();
    void connectButtons();
};

#endif // AUDITORIA_H
