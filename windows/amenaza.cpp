#include "amenaza.h"
#include "ui_amenaza.h"
#include "database/dbmanager.h"
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QMessageBox>
#include <QInputDialog>
#include <QFileDialog>
#include <QTextStream>
#include <QHeaderView>
#include <QSizePolicy>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QSqlTableModel>

amenaza::amenaza(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::amenaza)
{
    ui->setupUi(this);
    m_dbmanager = &dbmanager::instance();

    if (!m_dbmanager->isOpen()) {
        QMessageBox::critical(this, "Error", "No hay conexión a la base de datos");
        return;
    }

    setupTableModel();
    connectButtons();
}

amenaza::~amenaza()
{
    delete ui;
}

void amenaza::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    if (ui->tableView->model()) {
        ui->tableView->resizeColumnsToContents();
    }
}

void amenaza::setupTableModel()
{
    QSqlTableModel *model = new QSqlTableModel(this, m_dbmanager->getDatabase());
    model->setTable("threats");
    model->setEditStrategy(QSqlTableModel::OnManualSubmit);

    model->setHeaderData(model->fieldIndex("description"), Qt::Horizontal, tr("Descripción"));
    model->setHeaderData(model->fieldIndex("creation_date"), Qt::Horizontal, tr("Fecha de creación"));

    if (!model->select()) {
        QMessageBox::critical(this, "Error", "No se pudieron cargar los datos: " + model->lastError().text());
        return;
    }

    ui->tableView->setModel(model);
    ui->tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tableView->setSelectionBehavior(QAbstractItemView::SelectRows);

    ui->tableView->setColumnHidden(model->fieldIndex("id"), true);
    ui->tableView->setColumnHidden(model->fieldIndex("created_by"), true);


    ui->tableView->setColumnHidden(model->fieldIndex("description"), false);
    ui->tableView->setColumnHidden(model->fieldIndex("creation_date"), true);

    ui->tableView->resizeColumnsToContents();
    ui->tableView->horizontalHeader()->setStretchLastSection(true);
}

void amenaza::connectButtons()
{
    connect(ui->pushButton, &QPushButton::clicked, this, &amenaza::on_pushButton_clicked);
    connect(ui->pushButton_2, &QPushButton::clicked, this, &amenaza::on_pushButton_2_clicked);
    connect(ui->pushButton_3, &QPushButton::clicked, this, &amenaza::on_pushButton_3_clicked);
    connect(ui->pushButton_4, &QPushButton::clicked, this, &amenaza::on_pushButton_4_clicked);
    connect(ui->pushButton_5, &QPushButton::clicked, this, &amenaza::on_pushButton_5_clicked);
}

void amenaza::on_pushButton_clicked()
{
    QString searchText = ui->lineEdit->text().trimmed();
    if (searchText.isEmpty()) {
        QSqlTableModel *model = qobject_cast<QSqlTableModel*>(ui->tableView->model());
        if (model) model->setFilter("");
        return;
    }

    QSqlTableModel *model = qobject_cast<QSqlTableModel*>(ui->tableView->model());
    if (!model) return;

    QString filter = QString("description LIKE '%%1%'").arg(searchText);
    model->setFilter(filter);

    if (!model->select()) {
        QMessageBox::critical(this, "Error", "Error al buscar amenazas: " + model->lastError().text());
    }
}

void amenaza::on_pushButton_2_clicked()
{
    bool ok;
    QString descripcion = QInputDialog::getText(this, "Agregar Amenaza",
                                                "Descripción de la amenaza:",
                                                QLineEdit::Normal, "", &ok);
    if (!ok || descripcion.isEmpty()) return;

    QSqlQuery query(m_dbmanager->getDatabase());
    query.prepare("INSERT INTO threats (description, created_by) VALUES (:desc, :user_id) RETURNING id");
    query.bindValue(":desc", descripcion);
    query.bindValue(":user_id", m_dbmanager->currentUserId());

    if (!query.exec()) {
        QMessageBox::critical(this, "Error", "No se pudo agregar la amenaza: " + query.lastError().text());
        return;
    }

    if (query.next()) {
        int newId = query.value(0).toInt();
        QSqlQuery auditQuery(m_dbmanager->getDatabase());
        auditQuery.prepare(
            "INSERT INTO audit_logs (user_id, action, affected_table, record_id, new_data) "
            "VALUES (:user_id, 'CREATE', 'threats', :record_id, :new_data)"
            );
        auditQuery.bindValue(":user_id", m_dbmanager->currentUserId());
        auditQuery.bindValue(":record_id", newId);
        auditQuery.bindValue(":new_data", QString("{\"description\":\"%1\"}").arg(descripcion));
        if (!auditQuery.exec()) {
            qDebug() << "Failed to log audit:" << auditQuery.lastError().text();
        }
    }

    QSqlTableModel *model = qobject_cast<QSqlTableModel*>(ui->tableView->model());
    if (model) model->select();

    QMessageBox::information(this, "Éxito", "Amenaza agregada correctamente.");
}

void amenaza::on_pushButton_3_clicked()
{
    QModelIndexList selected = ui->tableView->selectionModel()->selectedRows();
    if (selected.isEmpty()) {
        QMessageBox::warning(this, "Advertencia", "Por favor seleccione una amenaza para editar.");
        return;
    }

    int row = selected.first().row();
    QSqlTableModel *model = qobject_cast<QSqlTableModel*>(ui->tableView->model());
    if (!model) return;

    QSqlRecord record = model->record(row);
    int id = record.value("id").toInt();
    QString oldDescription = record.value("description").toString();

    bool ok;
    QString newDescription = QInputDialog::getText(this, "Editar Amenaza", "Descripción:",
                                                   QLineEdit::Normal, oldDescription, &ok);
    if (!ok || newDescription.isEmpty()) return;

    QSqlQuery query(m_dbmanager->getDatabase());
    query.prepare("UPDATE threats SET description = :desc WHERE id = :id");
    query.bindValue(":desc", newDescription);
    query.bindValue(":id", id);

    if (!query.exec()) {
        QMessageBox::critical(this, "Error", "No se pudo actualizar la amenaza: " + query.lastError().text());
        return;
    }

    QSqlQuery auditQuery(m_dbmanager->getDatabase());
    auditQuery.prepare(
        "INSERT INTO audit_logs (user_id, action, affected_table, record_id, previous_data, new_data) "
        "VALUES (:user_id, 'UPDATE', 'threats', :record_id, :previous_data, :new_data)"
        );
    auditQuery.bindValue(":user_id", m_dbmanager->currentUserId());
    auditQuery.bindValue(":record_id", id);
    auditQuery.bindValue(":previous_data", QString("{\"description\":\"%1\"}").arg(oldDescription));
    auditQuery.bindValue(":new_data", QString("{\"description\":\"%1\"}").arg(newDescription));
    if (!auditQuery.exec()) {
        qDebug() << "Failed to log audit:" << auditQuery.lastError().text();
    }

    model->select();
    QMessageBox::information(this, "Éxito", "Amenaza actualizada correctamente.");
}

void amenaza::on_pushButton_4_clicked()
{
    QModelIndexList selected = ui->tableView->selectionModel()->selectedRows();
    if (selected.isEmpty()) {
        QMessageBox::warning(this, "Advertencia", "Por favor seleccione una amenaza para eliminar.");
        return;
    }

    int row = selected.first().row();
    QSqlTableModel *model = qobject_cast<QSqlTableModel*>(ui->tableView->model());
    if (!model) return;

    QSqlRecord record = model->record(row);
    int id = record.value("id").toInt();
    QString descripcion = record.value("description").toString();

    QMessageBox::StandardButton confirm;
    confirm = QMessageBox::question(this, "Confirmar eliminación",
                                    QString("¿Está seguro que desea eliminar la amenaza '%1'?").arg(descripcion),
                                    QMessageBox::Yes | QMessageBox::No);

    if (confirm != QMessageBox::Yes) return;

    QSqlQuery selectQuery(m_dbmanager->getDatabase());
    selectQuery.prepare("SELECT * FROM threats WHERE id = :id");
    selectQuery.bindValue(":id", id);
    if (!selectQuery.exec() || !selectQuery.next()) {
        QMessageBox::critical(this, "Error", "No se pudo obtener datos de la amenaza para auditoría: " + selectQuery.lastError().text());
        return;
    }
    QString previousData = QString("{\"description\":\"%1\"}").arg(selectQuery.value("description").toString());

    QSqlQuery query(m_dbmanager->getDatabase());
    query.prepare("DELETE FROM threats WHERE id = :id");
    query.bindValue(":id", id);

    if (!query.exec()) {
        QMessageBox::critical(this, "Error", "No se pudo eliminar la amenaza: " + query.lastError().text());
        return;
    }

    QSqlQuery auditQuery(m_dbmanager->getDatabase());
    auditQuery.prepare(
        "INSERT INTO audit_logs (user_id, action, affected_table, record_id, previous_data) "
        "VALUES (:user_id, 'DELETE', 'threats', :record_id, :previous_data)"
        );
    auditQuery.bindValue(":user_id", m_dbmanager->currentUserId());
    auditQuery.bindValue(":record_id", id);
    auditQuery.bindValue(":previous_data", previousData);
    if (!auditQuery.exec()) {
        qDebug() << "Failed to log audit:" << auditQuery.lastError().text();
    }

    model->select();
    QMessageBox::information(this, "Éxito", "Amenaza eliminada correctamente.");
}

void amenaza::on_pushButton_5_clicked()
{
    exportdialog dialog(this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    QString fileName = QFileDialog::getSaveFileName(this, "Exportar datos",
                                                    QDir::homePath(),
                                                    "Archivos PDF (*.pdf);;Archivos CSV (*.csv);;Archivos Excel (*.xlsx);;Archivos JSON (*.json)");
    if (fileName.isEmpty()) {
        return;
    }

    switch (dialog.selectedFormat()) {
    case exportdialog::CSV:
        exportToCSV(fileName);
        break;
    default:
        exportdialog::JSON;
        exportToJSON(fileName);
        break;
    }
}

void amenaza::exportToCSV(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Error", "No se pudo crear el archivo: " + file.errorString());
        return;
    }

    QTextStream out(&file);
    QSqlTableModel *model = qobject_cast<QSqlTableModel*>(ui->tableView->model());
    if (!model) return;

    // Write headers
    for (int i = 0; i < model->columnCount(); ++i) {
        out << '"' << model->headerData(i, Qt::Horizontal).toString().replace('"', "\"\"") << '"';
        if (i < model->columnCount() - 1)
            out << ",";
        else
            out << "\n";
    }

    // Write data
    for (int row = 0; row < model->rowCount(); ++row) {
        for (int col = 0; col < model->columnCount(); ++col) {
            QModelIndex index = model->index(row, col);
            out << '"' << model->data(index).toString().replace('"', "\"\"") << '"';
            if (col < model->columnCount() - 1)
                out << ",";
            else
                out << "\n";
        }
    }

    file.close();
    QMessageBox::information(this, "Éxito", "Datos exportados correctamente a CSV");
}

void amenaza::exportToJSON(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Error", "No se pudo crear el archivo: " + file.errorString());
        return;
    }

    QTextStream out(&file);
    QSqlTableModel *model = qobject_cast<QSqlTableModel*>(ui->tableView->model());
    if (!model) return;

    QJsonArray jsonArray;

    for (int row = 0; row < model->rowCount(); ++row) {
        QJsonObject jsonObj;
        for (int col = 0; col < model->columnCount(); ++col) {
            QString header = model->headerData(col, Qt::Horizontal).toString();
            QModelIndex index = model->index(row, col);
            jsonObj[header] = model->data(index).toString();
        }
        jsonArray.append(jsonObj);
    }

    QJsonDocument doc(jsonArray);
    out << doc.toJson();

    file.close();
    QMessageBox::information(this, "Éxito", "Datos exportados correctamente a JSON");
}
