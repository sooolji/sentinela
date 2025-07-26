#include "login.h"
#include "ui_login.h"
#include <QMessageBox>
#include <QSqlQuery>

login::login(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::login)
{
    ui->setupUi(this);
    setWindowTitle("Iniciar Sesión");

    connect(ui->showPasswordCheckBox, &QCheckBox::checkStateChanged,
            this, &login::on_showPasswordCheckBox_stateChanged);
}

login::~login()
{
    delete ui;
}

void login::on_showPasswordCheckBox_stateChanged(Qt::CheckState state)
{
    ui->passwordInput->setEchoMode(state == Qt::Checked ? QLineEdit::Normal : QLineEdit::Password);
}

void login::on_loginButton_clicked()
{
    QString username = ui->usernameInput->text().trimmed();
    QString password = ui->passwordInput->text();

    if (username.isEmpty() || password.isEmpty()) {
        QMessageBox::warning(this, "Error", "Usuario y contraseña son requeridos");
        return;
    }

    dbmanager& dbManager = dbmanager::instance();

    if (!dbManager.isOpen()) {
        QMessageBox::critical(this, "Error", "Error de conexión a la base de datos");
        return;
    }

    QSqlQuery query(dbManager.getDatabase());
    query.prepare("SELECT id, is_admin FROM users WHERE username = ? AND password = crypt(?, password)");
    query.addBindValue(username);
    query.addBindValue(password);

    if (!query.exec()) {
        QMessageBox::critical(this, "Error", "Error al verificar credenciales");
        return;
    }

    if (query.next()) {
        m_userId = query.value(0).toInt();
        m_isAdmin = query.value(1).toBool();

        // Registrar el inicio de sesión en la auditoría
        QSqlQuery auditQuery(dbManager.getDatabase());
        auditQuery.prepare("INSERT INTO audit_logs (user_id, action, affected_table, record_id) "
                           "VALUES (?, 'LOGIN', 'users', ?)");
        auditQuery.addBindValue(m_userId);
        auditQuery.addBindValue(m_userId);

        if (!auditQuery.exec()) {
            qDebug() << "Error al registrar auditoría de login:" << auditQuery.lastError().text();
        }

        dbManager.setCurrentUserId(m_userId);
        accept();
    } else {
        QMessageBox::warning(this, "Error", "Credenciales incorrectas");
        ui->passwordInput->clear();
        ui->usernameInput->setFocus();
    }
}
