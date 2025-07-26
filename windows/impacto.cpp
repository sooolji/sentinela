#include "impacto.h"
#include "ui_impacto.h"
#include "database/dbmanager.h"
#include <QMessageBox>
#include <QInputDialog>
#include <QFileDialog>
#include <QTextStream>
#include <QHeaderView>
#include <QSizePolicy>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlTableModel>
#include <QJsonDocument>
#include "dialogs/exportdialog.h"
#include <QtPrintSupport>

impacto::impacto(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::impacto)
    , m_dbManager(&dbmanager::instance())
{
    ui->setupUi(this);

    if (!m_dbManager->isOpen()) {
        QMessageBox::critical(this, "Error", "No hay conexión a la base de datos");
        return;
    }

    setupTableModel();
    connectButtons();
}

impacto::~impacto()
{
    delete ui;
}

void impacto::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    if (ui->tableView->model()) {
        ui->tableView->resizeColumnsToContents();
    }
}

void impacto::setupTableModel()
{
    QSqlQueryModel *model = new QSqlQueryModel(this);
    model->setQuery("SELECT a.id, a.description AS asset, t.description AS threat, "
                    "at.probability, (a.value * at.probability) AS risk_value "
                    "FROM assets_threats at "
                    "JOIN assets a ON at.asset_id = a.id "
                    "JOIN threats t ON at.threat_id = t.id",
                    m_dbManager->getDatabase());

    if (model->lastError().isValid()) {
        QMessageBox::critical(this, "Error", "Can't load data: " + model->lastError().text());
        return;
    }

    model->setHeaderData(1, Qt::Horizontal, tr("Activo"));
    model->setHeaderData(2, Qt::Horizontal, tr("Amenaza"));
    model->setHeaderData(3, Qt::Horizontal, tr("Probabilidad"));
    model->setHeaderData(4, Qt::Horizontal, tr("Valor de Riesgo"));

    ui->tableView->setModel(model);
    ui->tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tableView->setSelectionBehavior(QAbstractItemView::SelectRows);

    ui->tableView->hideColumn(0);

    ui->tableView->resizeColumnsToContents();
    ui->tableView->horizontalHeader()->setStretchLastSection(true);
}

void impacto::connectButtons()
{
    connect(ui->pushButton, &QPushButton::clicked, this, &impacto::onSearchClicked);
    connect(ui->pushButton_2, &QPushButton::clicked, this, &impacto::onAddClicked);
    connect(ui->pushButton_3, &QPushButton::clicked, this, &impacto::onEditClicked);
    connect(ui->pushButton_4, &QPushButton::clicked, this, &impacto::onDeleteClicked);
    connect(ui->pushButton_5, &QPushButton::clicked, this, &impacto::onExportClicked);
}

void impacto::onSearchClicked()
{
    QString filtro = ui->lineEdit->text().trimmed();
    QSqlQueryModel *model = qobject_cast<QSqlQueryModel*>(ui->tableView->model());

    if (!model) return;

    QString queryStr = "SELECT a.id, a.description AS asset, t.description AS threat, "
                       "at.probability, (a.value * at.probability) AS risk_value "
                       "FROM assets_threats at "
                       "JOIN assets a ON at.asset_id = a.id "
                       "JOIN threats t ON at.threat_id = t.id ";

    if (!filtro.isEmpty()) {
        queryStr += "WHERE a.description LIKE '%" + filtro + "%' OR t.description LIKE '%" + filtro + "%'";
    }

    model->setQuery(queryStr, m_dbManager->getDatabase());
    ui->tableView->resizeColumnsToContents();
}

void impacto::onAddClicked()
{
    QSqlQuery queryAssets("SELECT id, description FROM assets", m_dbManager->getDatabase());
    QStringList assets;
    QMap<QString, int> assetMap;

    while (queryAssets.next()) {
        QString desc = queryAssets.value(1).toString();
        assets << desc;
        assetMap[desc] = queryAssets.value(0).toInt();
    }

    QSqlQuery queryThreats("SELECT id, description FROM threats", m_dbManager->getDatabase());
    QStringList threats;
    QMap<QString, int> threatMap;

    while (queryThreats.next()) {
        QString desc = queryThreats.value(1).toString();
        threats << desc;
        threatMap[desc] = queryThreats.value(0).toInt();
    }

    if (assets.isEmpty() || threats.isEmpty()) {
        QMessageBox::warning(this, "Warning", "Debe existir al menos un activo y una amenaza para crear una relación.");
        return;
    }

    bool ok;
    QString asset = QInputDialog::getItem(this, "Seleccionar Activo", "Activo:", assets, 0, false, &ok);
    if (!ok) return;

    QString threat = QInputDialog::getItem(this, "Seleccionar Amenaza", "Amenaza:", threats, 0, false, &ok);
    if (!ok) return;

    double probability = QInputDialog::getDouble(this, "Probabilidad", "Ingrese la probabilidad (0.0 - 1.0):",
                                                 0.5, 0.0, 1.0, 1, &ok);
    if (!ok) return;

    QSqlQuery query(m_dbManager->getDatabase());
    query.prepare("INSERT INTO assets_threats (asset_id, threat_id, probability, modified_by) "
                  "VALUES (:asset_id, :threat_id, :probability, :user_id)");
    query.bindValue(":asset_id", assetMap[asset]);
    query.bindValue(":threat_id", threatMap[threat]);
    query.bindValue(":probability", probability);
    query.bindValue(":user_id", m_dbManager->currentUserId());

    if (!query.exec()) {
        QMessageBox::critical(this, "Error", "No se pudo crear la relación: " + query.lastError().text());
        return;
    }

    QSqlQuery auditQuery(m_dbManager->getDatabase());
    auditQuery.prepare("SELECT register_action(:user_id, 'CREATE', 'assets_threats', NULL, NULL, :new_data)");
    auditQuery.bindValue(":user_id", m_dbManager->currentUserId());
    auditQuery.bindValue(":new_data", QString("{\"asset\":\"%1\",\"threat\":\"%2\",\"probability\":%3}")
                                          .arg(asset).arg(threat).arg(probability));
    auditQuery.exec();

    setupTableModel();
    QMessageBox::information(this, "Éxito", "Relación activo-amenaza creada correctamente.");
}

void impacto::onEditClicked()
{
    QModelIndexList selected = ui->tableView->selectionModel()->selectedRows();
    if (selected.isEmpty()) {
        QMessageBox::warning(this, "Advertencia", "Por favor seleccione una relación para editar.");
        return;
    }

    int row = selected.first().row();
    QSqlQueryModel *model = qobject_cast<QSqlQueryModel*>(ui->tableView->model());
    if (!model) return;

    QString asset = model->data(model->index(row, 1)).toString();
    QString threat = model->data(model->index(row, 2)).toString();
    double currentProbability = model->data(model->index(row, 3)).toDouble();

    bool ok;
    double newProbability = QInputDialog::getDouble(this, "Editar Probabilidad",
                                                    "Nueva probabilidad (0.0 - 1.0):",
                                                    currentProbability, 0.0, 1.0, 1, &ok);
    if (!ok) return;

    QString assetIdQuery = QString("SELECT a.id, t.id FROM assets a, threats t "
                                   "WHERE a.description = '%1' AND t.description = '%2'").arg(asset, threat);
    QSqlQuery idQuery(assetIdQuery, m_dbManager->getDatabase());

    if (!idQuery.next()) {
        QMessageBox::critical(this, "Error", "No se pudo encontrar la relación.");
        return;
    }

    int assetId = idQuery.value(0).toInt();
    int threatId = idQuery.value(1).toInt();

    QSqlQuery query(m_dbManager->getDatabase());
    query.prepare("UPDATE assets_threats SET probability = :prob, "
                  "modified_by = :user_id, modification_date = CURRENT_TIMESTAMP "
                  "WHERE asset_id = :asset_id AND threat_id = :threat_id");
    query.bindValue(":prob", newProbability);
    query.bindValue(":asset_id", assetId);
    query.bindValue(":threat_id", threatId);
    query.bindValue(":user_id", m_dbManager->currentUserId());

    if (!query.exec()) {
        QMessageBox::critical(this, "Error", "No se pudo actualizar la probabilidad: " + query.lastError().text());
        return;
    }

    QSqlQuery auditQuery(m_dbManager->getDatabase());
    auditQuery.prepare("SELECT register_action(:user_id, 'UPDATE', 'assets_threats', NULL, :old_data, :new_data)");
    auditQuery.bindValue(":user_id", m_dbManager->currentUserId());
    auditQuery.bindValue(":old_data", QString("{\"probability\":%1}").arg(currentProbability));
    auditQuery.bindValue(":new_data", QString("{\"probability\":%1}").arg(newProbability));
    auditQuery.exec();

    setupTableModel();
    QMessageBox::information(this, "Éxito", "Probabilidad actualizada correctamente.");
}

void impacto::onDeleteClicked()
{
    QModelIndexList selected = ui->tableView->selectionModel()->selectedRows();
    if (selected.isEmpty()) {
        QMessageBox::warning(this, "Advertencia", "Por favor seleccione una relación para eliminar.");
        return;
    }

    int row = selected.first().row();
    QSqlQueryModel *model = qobject_cast<QSqlQueryModel*>(ui->tableView->model());
    if (!model) return;

    QString asset = model->data(model->index(row, 1)).toString();
    QString threat = model->data(model->index(row, 2)).toString();
    double probability = model->data(model->index(row, 3)).toDouble();

    QMessageBox::StandardButton confirm;
    confirm = QMessageBox::question(this, "Confirmar eliminación",
                                    QString("¿Está seguro que desea eliminar la relación entre '%1' y '%2'?").arg(asset, threat),
                                    QMessageBox::Yes | QMessageBox::No);

    if (confirm != QMessageBox::Yes) return;

    QString assetIdQuery = QString("SELECT a.id, t.id FROM assets a, threats t "
                                   "WHERE a.description = '%1' AND t.description = '%2'").arg(asset, threat);
    QSqlQuery idQuery(assetIdQuery, m_dbManager->getDatabase());

    if (!idQuery.next()) {
        QMessageBox::critical(this, "Error", "No se pudo encontrar la relación.");
        return;
    }

    int assetId = idQuery.value(0).toInt();
    int threatId = idQuery.value(1).toInt();

    QSqlQuery query(m_dbManager->getDatabase());
    query.prepare("DELETE FROM assets_threats WHERE asset_id = :asset_id AND threat_id = :threat_id");
    query.bindValue(":asset_id", assetId);
    query.bindValue(":threat_id", threatId);

    if (!query.exec()) {
        QMessageBox::critical(this, "Error", "No se pudo eliminar la relación: " + query.lastError().text());
        return;
    }

    QSqlQuery auditQuery(m_dbManager->getDatabase());
    auditQuery.prepare("SELECT register_action(:user_id, 'DELETE', 'assets_threats', NULL, :old_data, NULL)");
    auditQuery.bindValue(":user_id", m_dbManager->currentUserId());
    auditQuery.bindValue(":old_data", QString("{\"asset\":\"%1\",\"threat\":\"%2\",\"probability\":%3}")
                                          .arg(asset).arg(threat).arg(probability));
    auditQuery.exec();

    setupTableModel();
    QMessageBox::information(this, "Éxito", "Relación eliminada correctamente.");
}

void impacto::exportToCSV(const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Error", "No se pudo crear el archivo CSV: " + file.errorString());
        return;
    }

    QTextStream out(&file);
    QSqlQueryModel *model = qobject_cast<QSqlQueryModel*>(ui->tableView->model());
    if (!model) {
        file.close();
        return;
    }

    for (int col = 1; col < model->columnCount(); ++col) {
        out << "\"" << model->headerData(col, Qt::Horizontal).toString().replace("\"", "\"\"") << "\"";
        if (col < model->columnCount() - 1) out << ",";
    }
    out << "\n";

    for (int row = 0; row < model->rowCount(); ++row) {
        for (int col = 1; col < model->columnCount(); ++col) {
            QString text = model->data(model->index(row, col)).toString();
            out << "\"" << text.replace("\"", "\"\"") << "\"";
            if (col < model->columnCount() - 1) out << ",";
        }
        out << "\n";
    }

    file.close();
}

void impacto::exportToJSON(const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Error", "No se pudo crear el archivo JSON: " + file.errorString());
        return;
    }

    QTextStream out(&file);
    QSqlQueryModel *model = qobject_cast<QSqlQueryModel*>(ui->tableView->model());
    if (!model) {
        file.close();
        return;
    }

    QJsonArray jsonArray;
    for (int row = 0; row < model->rowCount(); ++row) {
        QJsonObject jsonObj;
        for (int col = 1; col < model->columnCount(); ++col) {
            QString header = model->headerData(col, Qt::Horizontal).toString();
            QVariant data = model->data(model->index(row, col));

            if (data.typeId() == QMetaType::Double || data.typeId() == QMetaType::Int) {
                jsonObj[header] = data.toDouble();
            } else {
                jsonObj[header] = data.toString();
            }
        }
        jsonArray.append(jsonObj);
    }

    QJsonDocument doc(jsonArray);
    out << doc.toJson(QJsonDocument::Indented); // Formato indentado para mejor legibilidad
    file.close();
}

void impacto::onExportClicked()
{
    exportdialog dialog(this);
    if (dialog.exec() != QDialog::Accepted) return;

    QString filter, defaultExtension, formatDescription;

    switch (dialog.selectedFormat()) {
    case exportdialog::CSV:
        filter = "Archivos CSV (*.csv)";
        defaultExtension = "csv";
        formatDescription = "CSV";
        break;
    case exportdialog::JSON:
        filter = "Archivos JSON (*.json)";
        defaultExtension = "json";
        formatDescription = "JSON";
        break;
    }

    QString fileName = QFileDialog::getSaveFileName(this, "Exportar datos",
                                                    QDir::homePath() + "/export." + defaultExtension,
                                                    filter);
    if (fileName.isEmpty()) return;

    switch (dialog.selectedFormat()) {
    case exportdialog::CSV:
        exportToCSV(fileName);
        break;
    case exportdialog::JSON:
        exportToJSON(fileName);
        break;
    }

    QSqlQuery auditQuery(m_dbManager->getDatabase());
    auditQuery.prepare("SELECT register_action(:user_id, 'EXPORT', 'assets_threats', NULL, NULL, :export_data)");
    auditQuery.bindValue(":user_id", m_dbManager->currentUserId());
    auditQuery.bindValue(":export_data",
                         QString("{\"export_type\":\"%1\",\"file_name\":\"%2\"}")
                             .arg(formatDescription)
                             .arg(QFileInfo(fileName).fileName()));
    auditQuery.exec();

    QMessageBox::information(this, "Éxito",
                             QString("Datos exportados correctamente como %1").arg(formatDescription));
}
