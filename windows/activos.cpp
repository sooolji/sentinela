#include "activos.h"
#include "ui_activos.h"
#include "dialogs/exportdialog.h"
#include "database/dbmanager.h"
#include <QMessageBox>
#include <QInputDialog>
#include <QFileDialog>
#include <QTextStream>
#include <QHeaderView>
#include <QSizePolicy>
#include <QSqlQuery>
#include <QSqlError>
#include <QSpinBox>
#include <QComboBox>
#include <QBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPageSize>
#include <QPageLayout>
#include <QDir>
#include <QTextDocument>

activos::activos(QWidget *parent, dbmanager* dbManager)
    : QWidget(parent)
    , ui(new Ui::activos)
    , m_dbManager(dbManager ? dbManager : &dbmanager::instance())
{
    ui->setupUi(this);


    if (!m_dbManager->isOpen()) {
        QMessageBox::critical(this, "Error", "No hay conexión a la base de datos");
        return;
    }

    setupTableModel();
}

activos::~activos()
{
    delete ui;
}

void activos::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    if (ui->tableView->model()) {
        ui->tableView->resizeColumnsToContents();
    }
}

void activos::setupTableModel()
{
    QSqlTableModel *model = new QSqlTableModel(this, m_dbManager->getDatabase());
    model->setTable("assets");
    model->setEditStrategy(QSqlTableModel::OnManualSubmit);

    model->setHeaderData(model->fieldIndex("description"), Qt::Horizontal, tr("Descripción"));
    model->setHeaderData(model->fieldIndex("type"), Qt::Horizontal, tr("Tipo"));
    model->setHeaderData(model->fieldIndex("location"), Qt::Horizontal, tr("Ubicación"));
    model->setHeaderData(model->fieldIndex("functionality"), Qt::Horizontal, tr("Función"));
    model->setHeaderData(model->fieldIndex("cost"), Qt::Horizontal, tr("Costo"));
    model->setHeaderData(model->fieldIndex("image"), Qt::Horizontal, tr("Imagen"));
    model->setHeaderData(model->fieldIndex("confidentiality"), Qt::Horizontal, tr("Confidencialidad"));
    model->setHeaderData(model->fieldIndex("integrity"), Qt::Horizontal, tr("Integridad"));
    model->setHeaderData(model->fieldIndex("availability"), Qt::Horizontal, tr("Disponibilidad"));
    model->setHeaderData(model->fieldIndex("value"), Qt::Horizontal, tr("Valoración"));

    if (!model->select()) {
        QMessageBox::critical(this, "Error", "No se pudieron cargar los datos: " + model->lastError().text());
        return;
    }

    ui->tableView->setModel(model);
    ui->tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tableView->setSelectionBehavior(QAbstractItemView::SelectRows);


    ui->tableView->setColumnHidden(model->fieldIndex("id"), true);
    ui->tableView->setColumnHidden(model->fieldIndex("entity_id"), true);
    ui->tableView->setColumnHidden(model->fieldIndex("created_by"), true);
    ui->tableView->setColumnHidden(model->fieldIndex("creation_date"), true);


    ui->tableView->setColumnHidden(model->fieldIndex("functionality"), false);
    ui->tableView->setColumnHidden(model->fieldIndex("cost"), false);
    ui->tableView->setColumnHidden(model->fieldIndex("image"), false);
    ui->tableView->setColumnHidden(model->fieldIndex("confidentiality"), false);
    ui->tableView->setColumnHidden(model->fieldIndex("integrity"), false);
    ui->tableView->setColumnHidden(model->fieldIndex("availability"), false);
    ui->tableView->setColumnHidden(model->fieldIndex("value"), false);


    ui->tableView->resizeColumnsToContents();
    ui->tableView->horizontalHeader()->setStretchLastSection(true);
}

void activos::on_pushButton_3_clicked()
{
    QModelIndexList selected = ui->tableView->selectionModel()->selectedRows();
    if (selected.isEmpty()) {
        QMessageBox::warning(this, "Advertencia", "Por favor seleccione un activo para editar.");
        return;
    }

    int row = selected.first().row();
    QSqlTableModel *model = qobject_cast<QSqlTableModel*>(ui->tableView->model());
    if (!model) return;

    QSqlRecord record = model->record(row);
    int id = record.value("id").toInt();

    QSqlQuery getOldValues(m_dbManager->getDatabase());
    getOldValues.prepare("SELECT row_to_json(assets) AS json_data FROM assets WHERE id = :id");
    getOldValues.bindValue(":id", id);
    if (!getOldValues.exec() || !getOldValues.next()) {
        QMessageBox::critical(this, "Error", "No se pudo obtener los valores actuales del activo.");
        return;
    }
    QString oldData = getOldValues.value("json_data").toString();

    QDialog dialog(this);
    dialog.setWindowTitle("Editar Activo");
    QFormLayout form(&dialog);

    QLineEdit *descripcionEdit = new QLineEdit(record.value("description").toString(), &dialog);
    QComboBox *tipoCombo = new QComboBox(&dialog);
    tipoCombo->addItems({"HW", "SW", "DT", "PR", "DO"});
    tipoCombo->setCurrentText(record.value("type").toString());
    QLineEdit *ubicacionEdit = new QLineEdit(record.value("location").toString(), &dialog);

    QSpinBox *funcionSpin = new QSpinBox(&dialog);
    funcionSpin->setRange(0, 10);
    funcionSpin->setValue(record.value("functionality").toInt());

    QSpinBox *costoSpin = new QSpinBox(&dialog);
    costoSpin->setRange(0, 10);
    costoSpin->setValue(record.value("cost").toInt());

    QSpinBox *imagenSpin = new QSpinBox(&dialog);
    imagenSpin->setRange(0, 10);
    imagenSpin->setValue(record.value("image").toInt());

    QSpinBox *confidencialidadSpin = new QSpinBox(&dialog);
    confidencialidadSpin->setRange(0, 10);
    confidencialidadSpin->setValue(record.value("confidentiality").toInt());

    QSpinBox *integridadSpin = new QSpinBox(&dialog);
    integridadSpin->setRange(0, 10);
    integridadSpin->setValue(record.value("integrity").toInt());

    QSpinBox *disponibilidadSpin = new QSpinBox(&dialog);
    disponibilidadSpin->setRange(0, 10);
    disponibilidadSpin->setValue(record.value("availability").toInt());

    form.addRow("Descripción:", descripcionEdit);
    form.addRow("Tipo:", tipoCombo);
    form.addRow("Ubicación:", ubicacionEdit);
    form.addRow(new QLabel("<b>Criterios de Valoración (0-10)</b>"));
    form.addRow("Función:", funcionSpin);
    form.addRow("Costo:", costoSpin);
    form.addRow("Imagen:", imagenSpin);
    form.addRow("Confidencialidad:", confidencialidadSpin);
    form.addRow("Integridad:", integridadSpin);
    form.addRow("Disponibilidad:", disponibilidadSpin);

    QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                               Qt::Horizontal, &dialog);
    form.addRow(&buttonBox);

    QObject::connect(&buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(&buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);


    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    if (descripcionEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "Advertencia", "La descripción no puede estar vacía.");
        return;
    }

    if (ubicacionEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "Advertencia", "La ubicación no puede estar vacía.");
        return;
    }

    QSqlDatabase::database().transaction();


    QSqlQuery query(m_dbManager->getDatabase());
    query.prepare("UPDATE assets SET "
                  "description = :desc, "
                  "type = :type, "
                  "location = :loc, "
                  "functionality = :func, "
                  "cost = :cost, "
                  "image = :img, "
                  "confidentiality = :conf, "
                  "integrity = :intg, "
                  "availability = :disp "
                  "WHERE id = :id");

    query.bindValue(":desc", descripcionEdit->text().trimmed());
    query.bindValue(":type", tipoCombo->currentText());
    query.bindValue(":loc", ubicacionEdit->text().trimmed());
    query.bindValue(":func", funcionSpin->value());
    query.bindValue(":cost", costoSpin->value());
    query.bindValue(":img", imagenSpin->value());
    query.bindValue(":conf", confidencialidadSpin->value());
    query.bindValue(":intg", integridadSpin->value());
    query.bindValue(":disp", disponibilidadSpin->value());
    query.bindValue(":id", id);

    if (!query.exec()) {
        QSqlDatabase::database().rollback();
        QMessageBox::critical(this, "Error", "No se pudo actualizar el activo: " + query.lastError().text());
        return;
    }


    QSqlQuery getNewValues(m_dbManager->getDatabase());
    getNewValues.prepare("SELECT row_to_json(assets) AS json_data FROM assets WHERE id = :id");
    getNewValues.bindValue(":id", id);
    if (!getNewValues.exec() || !getNewValues.next()) {
        QSqlDatabase::database().rollback();
        QMessageBox::critical(this, "Error", "No se pudo obtener los nuevos valores del activo.");
        return;
    }
    QString newData = getNewValues.value("json_data").toString();


    QSqlQuery auditQuery(m_dbManager->getDatabase());
    auditQuery.prepare("SELECT register_action(:user_id, 'UPDATE', 'assets', :record_id, "
                       "(:old_data)::jsonb, (:new_data)::jsonb)");
    auditQuery.bindValue(":user_id", m_dbManager->currentUserId());
    auditQuery.bindValue(":record_id", id);
    auditQuery.bindValue(":old_data", oldData);
    auditQuery.bindValue(":new_data", newData);

    if (!auditQuery.exec()) {
        QSqlDatabase::database().rollback();
        QMessageBox::critical(this, "Error", "No se pudo registrar la acción de auditoría: " + auditQuery.lastError().text());
        return;
    }

    QSqlDatabase::database().commit();
    model->select();
    QMessageBox::information(this, "Éxito", "Activo actualizado correctamente.");
}

void activos::on_pushButton_6_clicked()
{
    exportdialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        QString userFullName = "Usuario Actual";

        switch (dialog.selectedFormat()) {
        case exportdialog::CSV:
            exportToCSV();
            break;
        case exportdialog::JSON:
            exportToJSON();
            break;
        }
    }
}

void activos::exportToCSV()
{
    QString fileName = QFileDialog::getSaveFileName(this, "Exportar a CSV",
                                                    QDir::homePath(), "Archivos CSV (*.csv)");
    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Error", "No se pudo crear el archivo: " + file.errorString());
        return;
    }

    QTextStream out(&file);

    QSqlTableModel *model = qobject_cast<QSqlTableModel*>(ui->tableView->model());
    if (!model) return;

    for (int i = 0; i < model->columnCount(); ++i) {
        if (!ui->tableView->isColumnHidden(i)) {
            out << model->headerData(i, Qt::Horizontal).toString();
            if (i < model->columnCount() - 1)
                out << ",";
            else
                out << "\n";
        }
    }

    // Escribir datos
    for (int row = 0; row < model->rowCount(); ++row) {
        bool first = true;
        for (int col = 0; col < model->columnCount(); ++col) {
            if (!ui->tableView->isColumnHidden(col)) {
                if (!first) out << ",";
                QModelIndex index = model->index(row, col);
                out << "\"" << model->data(index).toString().replace("\"", "\"\"") << "\"";
                first = false;
            }
        }
        out << "\n";
    }

    file.close();
    QMessageBox::information(this, "Éxito", "Datos exportados correctamente a " + fileName);
}

void activos::exportToJSON()
{
    QString fileName = QFileDialog::getSaveFileName(this, "Exportar a JSON",
                                                    QDir::homePath(), "Archivos JSON (*.json)");
    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Error", "No se pudo crear el archivo: " + file.errorString());
        return;
    }

    QSqlTableModel *model = qobject_cast<QSqlTableModel*>(ui->tableView->model());
    if (!model) return;

    QJsonArray jsonArray;

    for (int row = 0; row < model->rowCount(); ++row) {
        QJsonObject jsonObj;
        for (int col = 0; col < model->columnCount(); ++col) {
            if (!ui->tableView->isColumnHidden(col)) {
                QModelIndex index = model->index(row, col);
                jsonObj[model->headerData(col, Qt::Horizontal).toString()] =
                    model->data(index).toString();
            }
        }
        jsonArray.append(jsonObj);
    }

    QJsonDocument doc(jsonArray);
    file.write(doc.toJson());
    file.close();

    QMessageBox::information(this, "Éxito", "Datos exportados correctamente a " + fileName);
}

void activos::on_pushButton_clicked()
{
    QString searchText = ui->lineEdit->text().trimmed();
    searchAssets(searchText);
}

void activos::on_pushButton_2_clicked()
{
    addNewAsset();
}

void activos::searchAssets(const QString& searchText)
{
    QSqlTableModel *model = qobject_cast<QSqlTableModel*>(ui->tableView->model());
    if (!model) return;

    QString filter = QString("description LIKE '%%1%' OR type LIKE '%%1%' OR location LIKE '%%1%'")
                         .arg(searchText);
    model->setFilter(filter);
    model->select();
}

void activos::addNewAsset()
{

    QDialog dialog(this);
    dialog.setWindowTitle("Nuevo Activo");
    QFormLayout form(&dialog);

    QLineEdit *descripcionEdit = new QLineEdit(&dialog);
    QComboBox *tipoCombo = new QComboBox(&dialog);
    tipoCombo->addItems({"HW", "SW", "DT", "PR", "DO"});
    QLineEdit *ubicacionEdit = new QLineEdit(&dialog);

    QSpinBox *funcionSpin = new QSpinBox(&dialog);
    funcionSpin->setRange(0, 10);
    funcionSpin->setValue(5);
    funcionSpin->setToolTip("Importancia de la tarea que cumple el activo");

    QSpinBox *costoSpin = new QSpinBox(&dialog);
    costoSpin->setRange(0, 10);
    costoSpin->setValue(5);
    costoSpin->setToolTip("Valor económico y de uso");

    QSpinBox *imagenSpin = new QSpinBox(&dialog);
    imagenSpin->setRange(0, 10);
    imagenSpin->setValue(5);
    imagenSpin->setToolTip("Impacto reputacional por pérdida");

    QSpinBox *confidencialidadSpin = new QSpinBox(&dialog);
    confidencialidadSpin->setRange(0, 10);
    confidencialidadSpin->setValue(5);
    confidencialidadSpin->setToolTip("Necesidad de protección de información");

    QSpinBox *integridadSpin = new QSpinBox(&dialog);
    integridadSpin->setRange(0, 10);
    integridadSpin->setValue(5);
    integridadSpin->setToolTip("Necesidad de prevenir modificaciones no autorizadas");

    QSpinBox *disponibilidadSpin = new QSpinBox(&dialog);
    disponibilidadSpin->setRange(0, 10);
    disponibilidadSpin->setValue(5);
    disponibilidadSpin->setToolTip("Requerimientos de acceso continuo");

    form.addRow("Descripción:", descripcionEdit);
    form.addRow("Tipo:", tipoCombo);
    form.addRow("Ubicación:", ubicacionEdit);

    form.addRow(new QLabel("<b>Criterios de Valoración (0-10)</b>"));
    form.addRow("Función:", funcionSpin);
    form.addRow("Costo:", costoSpin);
    form.addRow("Imagen:", imagenSpin);
    form.addRow("Confidencialidad:", confidencialidadSpin);
    form.addRow("Integridad:", integridadSpin);
    form.addRow("Disponibilidad:", disponibilidadSpin);


    QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                               Qt::Horizontal, &dialog);
    form.addRow(&buttonBox);

    QObject::connect(&buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(&buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);


    if (dialog.exec() != QDialog::Accepted) {
        return;
    }


    if (descripcionEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "Advertencia", "La descripción no puede estar vacía.");
        return;
    }

    if (ubicacionEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "Advertencia", "La ubicación no puede estar vacía.");
        return;
    }

    QSqlDatabase::database().transaction();


    QSqlQuery query(m_dbManager->getDatabase());
    query.prepare("INSERT INTO assets (description, type, location, functionality, cost, image, "
                  "confidentiality, integrity, availability, entity_id, created_by) "
                  "VALUES (:desc, :type, :loc, :func, :cost, :img, :conf, :intg, :disp, :ent, :user)");

    query.bindValue(":desc", descripcionEdit->text().trimmed());
    query.bindValue(":type", tipoCombo->currentText());
    query.bindValue(":loc", ubicacionEdit->text().trimmed());
    query.bindValue(":func", funcionSpin->value());
    query.bindValue(":cost", costoSpin->value());
    query.bindValue(":img", imagenSpin->value());
    query.bindValue(":conf", confidencialidadSpin->value());
    query.bindValue(":intg", integridadSpin->value());
    query.bindValue(":disp", disponibilidadSpin->value());
    query.bindValue(":ent", 1); // Asumir entity_id=1 por ahora
    query.bindValue(":user", m_dbManager->currentUserId());

    if (!query.exec()) {
        QSqlDatabase::database().rollback();
        QMessageBox::critical(this, "Error", "No se pudo agregar el activo: " + query.lastError().text());
        return;
    }


    int newId = query.lastInsertId().toInt();
    QSqlQuery auditQuery(m_dbManager->getDatabase());
    auditQuery.prepare("SELECT register_action(:user_id, 'CREATE', 'assets', :record_id, NULL, "
                       "(SELECT to_jsonb(row_to_json(assets)) FROM assets WHERE id = :record_id))");
    auditQuery.bindValue(":user_id", m_dbManager->currentUserId());
    auditQuery.bindValue(":record_id", newId);

    if (!auditQuery.exec()) {
        QSqlDatabase::database().rollback();
        QMessageBox::critical(this, "Error", "No se pudo registrar la acción de auditoría: " + auditQuery.lastError().text());
        return;
    }

    QSqlDatabase::database().commit();


    QSqlTableModel *model = qobject_cast<QSqlTableModel*>(ui->tableView->model());
    if (model) {
        model->select();
    }

    QMessageBox::information(this, "Éxito", "Activo agregado correctamente.");
}

void activos::on_pushButton_5_clicked()
{
    QModelIndexList selected = ui->tableView->selectionModel()->selectedRows();
    if (selected.isEmpty()) {
        QMessageBox::warning(this, "Advertencia", "Por favor seleccione un activo para eliminar.");
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
                                    QString("¿Está seguro que desea eliminar el activo '%1'?").arg(descripcion),
                                    QMessageBox::Yes | QMessageBox::No);

    if (confirm != QMessageBox::Yes) return;

    QSqlQuery getOldValues(m_dbManager->getDatabase());
    getOldValues.prepare("SELECT * FROM assets WHERE id = :id");
    getOldValues.bindValue(":id", id);
    if (!getOldValues.exec() || !getOldValues.next()) {
        QMessageBox::critical(this, "Error", "No se pudo obtener los valores del activo para auditoría.");
        return;
    }
    QSqlRecord oldRecord = getOldValues.record();

    QSqlDatabase::database().transaction();

    QSqlQuery query(m_dbManager->getDatabase());
    query.prepare("DELETE FROM assets WHERE id = :id");
    query.bindValue(":id", id);

    if (!query.exec()) {
        QSqlDatabase::database().rollback();
        QMessageBox::critical(this, "Error", "No se pudo eliminar el activo: " + query.lastError().text());
        return;
    }


    QSqlQuery auditQuery(m_dbManager->getDatabase());
    auditQuery.prepare("SELECT register_action(:user_id, 'DELETE', 'assets', :record_id, "
                       ":old_data::jsonb, NULL)");
    auditQuery.bindValue(":user_id", m_dbManager->currentUserId());
    auditQuery.bindValue(":record_id", id);

    QVariantMap oldData;
    for(int i = 0; i < oldRecord.count(); i++) {
        oldData[oldRecord.fieldName(i)] = oldRecord.value(i);
    }

    QJsonDocument jsonDoc = QJsonDocument::fromVariant(oldData);
    QString jsonString = jsonDoc.toJson(QJsonDocument::Compact);
    auditQuery.bindValue(":old_data", jsonString);

    if (!auditQuery.exec()) {
        QSqlDatabase::database().rollback();
        QMessageBox::critical(this, "Error", "No se pudo registrar la acción de auditoría: " + auditQuery.lastError().text());
        return;
    }

    QSqlDatabase::database().commit();
    model->select();
    QMessageBox::information(this, "Éxito", "Activo eliminado correctamente.");
}
