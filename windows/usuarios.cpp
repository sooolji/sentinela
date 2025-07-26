#include "usuarios.h"
#include "ui_usuarios.h"
#include "dialogs/userdialog.h"
#include "dialogs/exportdialog.h"
#include <QMessageBox>
#include <QSqlTableModel>
#include <QInputDialog>
#include <QSqlQuery>
#include <QSqlError>
#include <QFileDialog>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QPrinter>
#include <QPainter>
#include <QPixmap>
#include <QDateTime>
#include <QDebug>


usuarios::usuarios(QWidget *parent) :
    QWidget(parent), ui(new Ui::usuarios), m_dbmanager(&dbmanager::instance())
{
    ui->setupUi(this);

    if (!m_dbmanager->isOpen()) {
        QMessageBox::critical(this, "Error", "No hay conexión a la base de datos");
        return;
    }

    setupTableModel();
}

usuarios::~usuarios()
{
    delete ui;
    delete model;
}

void usuarios::setupTableModel()
{
    model = new QSqlTableModel(this, m_dbmanager->getDatabase());
    model->setTable("users");
    model->setEditStrategy(QSqlTableModel::OnManualSubmit);

    model->setHeaderData(model->fieldIndex("first_name"), Qt::Horizontal, tr("Nombre"));
    model->setHeaderData(model->fieldIndex("last_name"), Qt::Horizontal, tr("Apellido"));
    model->setHeaderData(model->fieldIndex("username"), Qt::Horizontal, tr("Usuario"));
    model->setHeaderData(model->fieldIndex("is_admin"), Qt::Horizontal, tr("Administrador"));

    if (!model->select()) {
        QMessageBox::critical(this, "Error", "No se pudieron cargar los datos: " + model->lastError().text());
        return;
    }

    ui->tableView->setModel(model);
    ui->tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    hideTechnicalColumns();
    ui->tableView->resizeColumnsToContents();
}

void usuarios::hideTechnicalColumns()
{
    if (!model) return;

    QVector<QString> columnsToHide = {"id", "entity_id", "creation_date", "password"};
    for (const auto& column : columnsToHide) {
        int colIndex = model->fieldIndex(column);
        if (colIndex != -1) {
            ui->tableView->setColumnHidden(colIndex, true);
        }
    }
}

void usuarios::on_pushButton_clicked() // Buscar
{
    QString searchText = ui->lineEdit->text().trimmed();
    if (searchText.isEmpty()) {
        model->setFilter("");
    } else {
        QString filter = QString("first_name LIKE ? OR last_name LIKE ? OR username LIKE ?");

        // Crear query directamente sin copiar
        QSqlQuery q(model->database());
        q.prepare("SELECT * FROM users WHERE " + filter);
        QString pattern = "%" + searchText + "%";
        q.addBindValue(pattern);
        q.addBindValue(pattern);
        q.addBindValue(pattern);

        if (!q.exec()) {
            QMessageBox::critical(this, "Error", "Error al buscar: " + q.lastError().text());
            return;
        }

        model->setQuery(std::move(q));
    }

    if (!model->select()) {
        QMessageBox::critical(this, "Error", "Error al buscar usuarios: " + model->lastError().text());
    }

    hideTechnicalColumns();
}

void usuarios::on_pushButton_2_clicked()
{
    UserDialog dialog(this, false);
    if (dialog.exec() == QDialog::Accepted) {
        QMap<QString, QVariant> userData = dialog.getUserData();

        if (userData["first_name"].toString().isEmpty() ||
            userData["last_name"].toString().isEmpty() ||
            userData["username"].toString().isEmpty()) {
            QMessageBox::warning(this, "Error", "Nombre, apellido y usuario son requeridos");
            return;
        }

        QSqlQuery entityCheck(m_dbmanager->getDatabase());
        entityCheck.prepare("SELECT COUNT(*) FROM entities WHERE name = :name");
        entityCheck.bindValue(":name", userData["entity_name"]);
        if (!entityCheck.exec() || !entityCheck.next()) {
            QMessageBox::critical(this, "Error", "Error al verificar entidad: " + entityCheck.lastError().text());
            return;
        }
        if (entityCheck.value(0).toInt() == 0) {
            QMessageBox::warning(this, "Error", "La entidad especificada no existe");
            return;
        }

        QSqlQuery checkQuery(m_dbmanager->getDatabase());
        checkQuery.prepare("SELECT COUNT(*) FROM users WHERE username = :username");
        checkQuery.bindValue(":username", userData["username"]);
        if (!checkQuery.exec() || !checkQuery.next()) {
            QMessageBox::critical(this, "Error", "Error al verificar usuario: " + checkQuery.lastError().text());
            return;
        }
        if (checkQuery.value(0).toInt() > 0) {
            QMessageBox::warning(this, "Error", "El nombre de usuario ya existe");
            return;
        }

        m_dbmanager->getDatabase().transaction();

        QSqlQuery query(m_dbmanager->getDatabase());
        query.prepare("INSERT INTO users (first_name, last_name, username, password, is_admin, entity_id) "
                      "VALUES (:first_name, :last_name, :username, crypt(:password, gen_salt('bf', 12)), :is_admin, "
                      "(SELECT id FROM entities WHERE name = :entity_name))");
        query.bindValue(":first_name", userData["first_name"]);
        query.bindValue(":last_name", userData["last_name"]);
        query.bindValue(":username", userData["username"]);
        query.bindValue(":password", userData["password"].toString());
        query.bindValue(":is_admin", userData["is_admin"]);
        query.bindValue(":entity_name", userData["entity_name"]);

        if (!query.exec()) {
            m_dbmanager->getDatabase().rollback();
            QMessageBox::critical(this, "Error", "Error al agregar usuario: " + query.lastError().text());
            return;
        }

        int newId = query.lastInsertId().toInt();

        QSqlQuery updateEntity(m_dbmanager->getDatabase());
        updateEntity.prepare("UPDATE entities SET address = :address WHERE name = :name");
        updateEntity.bindValue(":address", userData["entity_address"]);
        updateEntity.bindValue(":name", userData["entity_name"]);
        if (!updateEntity.exec()) {
            m_dbmanager->getDatabase().rollback();
            QMessageBox::critical(this, "Error", "Error al actualizar entidad: " + updateEntity.lastError().text());
            return;
        }

        if (!m_dbmanager->getDatabase().commit()) {
            QMessageBox::critical(this, "Error", "Error al guardar los cambios: " + m_dbmanager->getDatabase().lastError().text());
            return;
        }

        QJsonObject newData;
        newData["first_name"] = userData["first_name"].toString();
        newData["last_name"] = userData["last_name"].toString();
        newData["username"] = userData["username"].toString();
        newData["is_admin"] = userData["is_admin"].toBool();
        newData["entity_name"] = userData["entity_name"].toString();
        logAction("CREATE", newId, "users", {}, newData);

        refreshTable();
        QMessageBox::information(this, "Éxito", "Usuario agregado correctamente");
    }
}

void usuarios::on_pushButton_3_clicked() // Editar
{
    QModelIndexList selected = ui->tableView->selectionModel()->selectedRows();
    if (selected.isEmpty()) {
        QMessageBox::warning(this, "Advertencia", "Por favor seleccione un usuario para editar.");
        return;
    }

    int row = selected.first().row();
    int userId = model->data(model->index(row, 0)).toInt();

    QMap<QString, QVariant> userData;
    userData["first_name"] = model->data(model->index(row, model->fieldIndex("first_name")));
    userData["last_name"] = model->data(model->index(row, model->fieldIndex("last_name")));
    userData["username"] = model->data(model->index(row, model->fieldIndex("username")));
    userData["is_admin"] = model->data(model->index(row, model->fieldIndex("is_admin")));

    QSqlQuery entityQuery(m_dbmanager->getDatabase());
    entityQuery.prepare("SELECT e.name, e.address FROM users u "
                        "JOIN entities e ON u.entity_id = e.id "
                        "WHERE u.id = :id");
    entityQuery.bindValue(":id", userId);
    if (entityQuery.exec() && entityQuery.next()) {
        userData["entity_name"] = entityQuery.value(0);
        userData["entity_address"] = entityQuery.value(1);
    }

    UserDialog dialog(this, true);
    dialog.setUserData(userData);

    if (dialog.exec() == QDialog::Accepted) {
        QMap<QString, QVariant> newData = dialog.getUserData();

        if (newData["first_name"].toString().isEmpty() ||
            newData["last_name"].toString().isEmpty() ||
            newData["username"].toString().isEmpty()) {
            QMessageBox::warning(this, "Error", "Nombre, apellido y usuario son requeridos");
            return;
        }

        QSqlQuery entityCheck(m_dbmanager->getDatabase());
        entityCheck.prepare("SELECT COUNT(*) FROM entities WHERE name = :name");
        entityCheck.bindValue(":name", newData["entity_name"]);
        if (!entityCheck.exec() || !entityCheck.next()) {
            QMessageBox::critical(this, "Error", "Error al verificar entidad: " + entityCheck.lastError().text());
            return;
        }
        if (entityCheck.value(0).toInt() == 0) {
            QMessageBox::warning(this, "Error", "La entidad especificada no existe");
            return;
        }

        QSqlQuery checkQuery(m_dbmanager->getDatabase());
        checkQuery.prepare("SELECT COUNT(*) FROM users WHERE username = :username AND id != :id");
        checkQuery.bindValue(":username", newData["username"]);
        checkQuery.bindValue(":id", userId);
        if (!checkQuery.exec() || !checkQuery.next()) {
            QMessageBox::critical(this, "Error", "Error al verificar usuario: " + checkQuery.lastError().text());
            return;
        }
        if (checkQuery.value(0).toInt() > 0) {
            QMessageBox::warning(this, "Error", "El nombre de usuario ya existe");
            return;
        }

        m_dbmanager->getDatabase().transaction();

        QString queryStr = "UPDATE users SET "
                           "first_name = :first_name, "
                           "last_name = :last_name, "
                           "username = :username, "
                           "is_admin = :is_admin, "
                           "entity_id = (SELECT id FROM entities WHERE name = :entity_name)";

        if (!newData["password"].toString().isEmpty()) {
            if (newData["password"] != newData["confirm_password"]) {
                QMessageBox::warning(this, "Error", "Las contraseñas no coinciden");
                m_dbmanager->getDatabase().rollback();
                return;
            }
            if (newData["password"].toString().length() < 6) {
                QMessageBox::warning(this, "Error", "La contraseña debe tener al menos 6 caracteres");
                m_dbmanager->getDatabase().rollback();
                return;
            }
            queryStr += ", password = crypt(:password, gen_salt('bf', 12))";
        }

        queryStr += " WHERE id = :id";

        QSqlQuery query(m_dbmanager->getDatabase());
        query.prepare(queryStr);
        query.bindValue(":first_name", newData["first_name"]);
        query.bindValue(":last_name", newData["last_name"]);
        query.bindValue(":username", newData["username"]);
        query.bindValue(":is_admin", newData["is_admin"]);
        query.bindValue(":entity_name", newData["entity_name"]);
        if (!newData["password"].toString().isEmpty()) {
            query.bindValue(":password", newData["password"].toString());
        }
        query.bindValue(":id", userId);

        if (!query.exec()) {
            m_dbmanager->getDatabase().rollback();
            QMessageBox::critical(this, "Error", "Error al actualizar usuario: " + query.lastError().text());
            return;
        }

        QSqlQuery updateEntity(m_dbmanager->getDatabase());
        updateEntity.prepare("UPDATE entities SET address = :address WHERE name = :name");
        updateEntity.bindValue(":address", newData["entity_address"]);
        updateEntity.bindValue(":name", newData["entity_name"]);
        if (!updateEntity.exec()) {
            m_dbmanager->getDatabase().rollback();
            QMessageBox::critical(this, "Error", "Error al actualizar entidad: " + updateEntity.lastError().text());
            return;
        }

        if (!m_dbmanager->getDatabase().commit()) {
            QMessageBox::critical(this, "Error", "Error al guardar los cambios: " + m_dbmanager->getDatabase().lastError().text());
            return;
        }

        QJsonObject oldData;
        oldData["first_name"] = userData["first_name"].toString();
        oldData["last_name"] = userData["last_name"].toString();
        oldData["username"] = userData["username"].toString();
        oldData["is_admin"] = userData["is_admin"].toBool();
        oldData["entity_name"] = userData["entity_name"].toString();

        QJsonObject newDataJson;
        newDataJson["first_name"] = newData["first_name"].toString();
        newDataJson["last_name"] = newData["last_name"].toString();
        newDataJson["username"] = newData["username"].toString();
        newDataJson["is_admin"] = newData["is_admin"].toBool();
        newDataJson["entity_name"] = newData["entity_name"].toString();

        logAction("UPDATE", userId, "users", oldData, newDataJson);

        refreshTable();
        QMessageBox::information(this, "Éxito", "Usuario actualizado correctamente");
    }
}

void usuarios::on_pushButton_4_clicked()
{
    QModelIndexList selected = ui->tableView->selectionModel()->selectedRows();
    if (selected.isEmpty()) {
        QMessageBox::warning(this, "Advertencia", "Por favor seleccione un usuario para eliminar.");
        return;
    }

    int row = selected.first().row();
    int userId = model->data(model->index(row, 0)).toInt();
    QString username = model->data(model->index(row, model->fieldIndex("username"))).toString();

    QMessageBox::StandardButton confirm;
    confirm = QMessageBox::question(this, "Confirmar eliminación",
                                    QString("¿Está seguro que desea eliminar al usuario '%1'?").arg(username),
                                    QMessageBox::Yes | QMessageBox::No);

    if (confirm != QMessageBox::Yes) return;

    QJsonObject oldData;
    oldData["first_name"] = model->data(model->index(row, model->fieldIndex("first_name"))).toString();
    oldData["last_name"] = model->data(model->index(row, model->fieldIndex("last_name"))).toString();
    oldData["username"] = model->data(model->index(row, model->fieldIndex("username"))).toString();
    oldData["is_admin"] = model->data(model->index(row, model->fieldIndex("is_admin"))).toBool();

    QSqlQuery entityQuery(m_dbmanager->getDatabase());
    entityQuery.prepare("SELECT e.name FROM users u "
                        "JOIN entities e ON u.entity_id = e.id "
                        "WHERE u.id = :id");
    entityQuery.bindValue(":id", userId);
    if (entityQuery.exec() && entityQuery.next()) {
        oldData["entity_name"] = entityQuery.value(0).toString();
    }

    QSqlQuery query(m_dbmanager->getDatabase());
    query.prepare("DELETE FROM users WHERE id = :id");
    query.bindValue(":id", userId);

    if (!query.exec()) {
        QMessageBox::critical(this, "Error", "No se pudo eliminar el usuario: " + query.lastError().text());
        return;
    }

    logAction("DELETE", userId, "users", oldData, {});

    refreshTable();
    QMessageBox::information(this, "Éxito", "Usuario eliminado correctamente");
}

void usuarios::refreshTable()
{
    if (!model->select()) {
        QMessageBox::critical(this, "Error", "Error al refrescar la tabla: " + model->lastError().text());
    }
    hideTechnicalColumns();
    ui->tableView->resizeColumnsToContents();
}

void usuarios::on_pushButton_5_clicked()
{
    logAction("EXPORT", 0, "users");

    exportdialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        exportdialog::ExportFormat format = dialog.selectedFormat();
        QString fileName = QFileDialog::getSaveFileName(this, "Guardar como", "",
        format == exportdialog::CSV ? "CSV Files (*.csv)" : "JSON Files (*.json)");

        if (fileName.isEmpty()) return;

        bool success = false;
        switch (format) {
        case exportdialog::CSV:
            success = exportToCsv(fileName);
            break;
        case exportdialog::JSON:
            success = exportToJson(fileName);
            break;
        }

        if (success) {
            QMessageBox::information(this, "Éxito", "Datos exportados correctamente");
        } else {
            QMessageBox::warning(this, "Error", "No se pudo exportar los datos");
        }
    }
}

bool usuarios::exportToCsv(const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;

    QTextStream out(&file);

    for (int col = 0; col < model->columnCount(); ++col) {
        if (!ui->tableView->isColumnHidden(col)) {
            out << '"' << model->headerData(col, Qt::Horizontal).toString().replace('"', "\"\"") << '"';
            if (col < model->columnCount() - 1)
                out << ",";
        }
    }
    out << "\n";

    for (int row = 0; row < model->rowCount(); ++row) {
        for (int col = 0; col < model->columnCount(); ++col) {
            if (!ui->tableView->isColumnHidden(col)) {
                out << '"' << model->data(model->index(row, col)).toString().replace('"', "\"\"") << '"';
                if (col < model->columnCount() - 1)
                    out << ",";
            }
        }
        out << "\n";
    }

    file.close();
    return true;
}

bool usuarios::exportToJson(const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;

    QJsonArray jsonArray;

    for (int row = 0; row < model->rowCount(); ++row) {
        QJsonObject jsonObj;
        for (int col = 0; col < model->columnCount(); ++col) {
            if (!ui->tableView->isColumnHidden(col)) {
                QString header = model->headerData(col, Qt::Horizontal).toString();
                jsonObj[header] = QJsonValue::fromVariant(model->data(model->index(row, col)));
            }
        }
        jsonArray.append(jsonObj);
    }

    QJsonDocument doc(jsonArray);
    file.write(doc.toJson());
    file.close();
    return true;
}

void usuarios::logAction(const QString& action, int record_id, const QString& table,
                         const QJsonObject& previous, const QJsonObject& current)
{
    QSqlQuery query(m_dbmanager->getDatabase());
    query.prepare("SELECT register_action(:user_id, :action, :table, :record_id, :previous, :current)");
    query.bindValue(":user_id", m_dbmanager->currentUserId());
    query.bindValue(":action", action);
    query.bindValue(":table", table);
    query.bindValue(":record_id", record_id);
    query.bindValue(":previous", previous.isEmpty() ? QVariant() : QJsonDocument(previous).toJson(QJsonDocument::Compact));
    query.bindValue(":current", current.isEmpty() ? QVariant() : QJsonDocument(current).toJson(QJsonDocument::Compact));

    if (!query.exec()) {
        qWarning() << "Error en auditoría:" << query.lastError().text();
    }
}
