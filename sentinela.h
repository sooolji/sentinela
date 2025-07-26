#ifndef SENTINELA_H
#define SENTINELA_H

#include <QMainWindow>
#include "windows/dashboard.h"
#include "windows/activos.h"
#include "windows/amenaza.h"
#include "windows/impacto.h"
#include "windows/usuarios.h"
#include "windows/auditoria.h"

namespace Ui {
class sentinela;
}

class sentinela : public QMainWindow
{
    Q_OBJECT

public:
    explicit sentinela(QWidget *parent = nullptr, int userId = 0, bool isAdmin = false);
    ~sentinela();

private slots:
    void switchTodashboard();
    void switchToAssets();
    void switchToThreats();
    void switchToRiskManagement();
    void switchToUsers();
    void switchToAudit();

private:
    Ui::sentinela *ui;
    int m_userId;
    bool m_isAdmin;
};

#endif // SENTINELA_H
