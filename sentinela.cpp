#include "sentinela.h"
#include "ui_sentinela.h"
#include <QMessageBox>
#include <QDebug>

sentinela::sentinela(QWidget *parent, int userId, bool isAdmin)
    : QMainWindow(parent),
    ui(new Ui::sentinela),
    m_userId(userId),
    m_isAdmin(isAdmin)
{
    ui->setupUi(this);

    ui->menuAdmin->setVisible(m_isAdmin);
    ui->menuAdmin->menuAction()->setVisible(m_isAdmin);

    connect(ui->actionInicio, &QAction::triggered, this, &sentinela::switchTodashboard);
    connect(ui->actionActivos, &QAction::triggered, this, &sentinela::switchToAssets);
    connect(ui->actionAmenazas, &QAction::triggered, this, &sentinela::switchToThreats);
    connect(ui->actionImpacto, &QAction::triggered, this, &sentinela::switchToRiskManagement);
    connect(ui->actionUsuarios, &QAction::triggered, this, &sentinela::switchToUsers);
    connect(ui->actionAuditoria, &QAction::triggered, this, &sentinela::switchToAudit);


    ui->stackedWidget->addWidget(new dashboard(this));
    ui->stackedWidget->addWidget(new activos(this));
    ui->stackedWidget->addWidget(new amenaza(this));
    ui->stackedWidget->addWidget(new impacto(this));

    if(m_isAdmin){
        ui->stackedWidget->addWidget(new usuarios(this));
        ui->stackedWidget->addWidget(new auditoria(this));
    }
}

sentinela::~sentinela(){
    delete ui;
}

void sentinela::switchTodashboard() {
    ui->stackedWidget->setCurrentIndex(0);
}

void sentinela::switchToAssets() {
    ui->stackedWidget->setCurrentIndex(1);
}

void sentinela::switchToThreats() {
    ui->stackedWidget->setCurrentIndex(2);
}

void sentinela::switchToRiskManagement() {
    ui->stackedWidget->setCurrentIndex(3);
}

void sentinela::switchToUsers() {
    ui->stackedWidget->setCurrentIndex(4);
}

void sentinela::switchToAudit() {
    ui->stackedWidget->setCurrentIndex(5);
}
