#include "userdialog.h"
#include "ui_userdialog.h"

#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QMessageBox>

UserDialog::UserDialog(QWidget *parent, bool isEditMode) :
    QDialog(parent),
    ui(new Ui::UserDialog),
    m_isEditMode(isEditMode)
{
    ui->setupUi(this);
    setWindowTitle(isEditMode ? "Editar Usuario" : "Agregar Usuario");

    connect(ui->checkBox, &QCheckBox::checkStateChanged,
            this, &UserDialog::on_showPasswordCheckBox_stateChanged);

    ui->passwordEdit->setEchoMode(QLineEdit::Password);
    ui->confirmPasswordEdit->setEchoMode(QLineEdit::Password);

    ui->adminSpinBox->setRange(0, 1);
    ui->adminSpinBox->setValue(0);

    if (isEditMode) {
        ui->passwordEdit->setPlaceholderText("Dejar en blanco para no cambiar");
        ui->confirmPasswordEdit->setPlaceholderText("Dejar en blanco para no cambiar");
        ui->adminSpinBox->setVisible(true);
    } else {
        ui->adminSpinBox->setVisible(false);
    }


    connect(ui->acceptButton, &QPushButton::clicked, this, [this]() {

        if (ui->firstNameEdit->text().trimmed().isEmpty() ||
            ui->lastNameEdit->text().trimmed().isEmpty() ||
            ui->usernameEdit->text().trimmed().isEmpty()) {
            QMessageBox::warning(this, "Error", "Nombre, apellido y usuario son requeridos");
            return;
        }


        if (!m_isEditMode || (!ui->passwordEdit->text().isEmpty() || !ui->confirmPasswordEdit->text().isEmpty())) {
            if (ui->passwordEdit->text().isEmpty()) {
                QMessageBox::warning(this, "Error", "La contraseña es requerida");
                return;
            }

            if (ui->passwordEdit->text() != ui->confirmPasswordEdit->text()) {
                QMessageBox::warning(this, "Error", "Las contraseñas no coinciden");
                return;
            }

            if (ui->passwordEdit->text().length() < 6) {
                QMessageBox::warning(this, "Error", "La contraseña debe tener al menos 6 caracteres");
                return;
            }
        }

        accept();
    });

    connect(ui->rejectButton, &QPushButton::clicked, this, &QDialog::reject);
}

UserDialog::~UserDialog()
{
    delete ui;
}

void UserDialog::setUserData(const QMap<QString, QVariant> &data)
{
    ui->firstNameEdit->setText(data.value("first_name").toString());
    ui->lastNameEdit->setText(data.value("last_name").toString());
    ui->usernameEdit->setText(data.value("username").toString());
    ui->adminSpinBox->setValue(data.value("is_admin", 0).toInt());
    ui->entityEdit->setText(data.value("entity_name").toString());
    ui->addressEdit->setText(data.value("entity_address").toString());
}

QMap<QString, QVariant> UserDialog::getUserData() const
{
    QMap<QString, QVariant> data;
    data["first_name"] = ui->firstNameEdit->text().trimmed();
    data["last_name"] = ui->lastNameEdit->text().trimmed();
    data["username"] = ui->usernameEdit->text().trimmed();
    data["password"] = ui->passwordEdit->text();
    data["confirm_password"] = ui->confirmPasswordEdit->text();
    data["is_admin"] = ui->adminSpinBox->value();
    data["entity_name"] = ui->entityEdit->text().trimmed();
    data["entity_address"] = ui->addressEdit->text().trimmed();
    return data;
}

void UserDialog::on_showPasswordCheckBox_stateChanged(Qt::CheckState state)
{
    ui->passwordEdit->setEchoMode(state == Qt::Checked ? QLineEdit::Normal : QLineEdit::Password);
    ui->confirmPasswordEdit->setEchoMode(state == Qt::Checked ? QLineEdit::Normal : QLineEdit::Password);
}



