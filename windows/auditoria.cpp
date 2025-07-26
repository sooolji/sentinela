#include "auditoria.h"
#include "ui_auditoria.h"
#include "database/dbmanager.h"
#include "dialogs/exportdialog.h"
#include <QMessageBox>
#include <QFileDialog>
#include <QTextStream>
#include <QHeaderView>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlQueryModel>
#include <QPrinter>
#include <QPainter>
#include <QProcess>
#include <QPdfWriter>
#include <QDesktopServices>
#include <stdexcept>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>

auditoria::auditoria(QWidget *parent, int userId)
    : QWidget(parent)
    , ui(new Ui::auditoria)
    , m_userId(userId)
{
    try {
        ui->setupUi(this);

        if (!parent) {
            qWarning() << "Warning: Auditoria window created without parent!";
        }

        if (!dbmanager::instance().isOpen()) {
            QMessageBox::critical(this, "Error", "No hay conexión a la base de datos");
            return;
        }

        setupTableModel();
        connectButtons();
    } catch (const std::exception &e) {
        QMessageBox::critical(nullptr, "Error", QString("Failed to create auditoria window: ") + e.what());
        throw;
    }
}

auditoria::~auditoria()
{
    delete ui;
}

void auditoria::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    if (ui->tableView->model()) {
        ui->tableView->resizeColumnsToContents();
    }
}

void auditoria::setupTableModel()
{
    QSqlQueryModel *model = new QSqlQueryModel(this);
    model->setQuery("SELECT al.id, "
                    "u.username AS usuario, "
                    "al.action AS accion, "
                    "al.affected_table AS tabla, "
                    "al.record_id AS registro, "
                    "TO_CHAR(al.event_date, 'DD/MM/YYYY HH24:MI') AS fecha, "
                    "CASE WHEN al.previous_data IS NULL THEN '' ELSE 'Sí' END AS datos_previos, "
                    "CASE WHEN al.new_data IS NULL THEN '' ELSE 'Sí' END AS datos_nuevos "
                    "FROM audit_logs al "
                    "LEFT JOIN users u ON al.user_id = u.id "
                    "ORDER BY al.event_date DESC",
                    dbmanager::instance().getDatabase());

    if (model->lastError().isValid()) {
        QMessageBox::critical(this, "Error", "No se pudieron cargar los datos: " + model->lastError().text());
        return;
    }

    model->setHeaderData(1, Qt::Horizontal, tr("Usuario"));
    model->setHeaderData(2, Qt::Horizontal, tr("Acción"));
    model->setHeaderData(3, Qt::Horizontal, tr("Tabla afectada"));
    model->setHeaderData(4, Qt::Horizontal, tr("ID Registro"));
    model->setHeaderData(5, Qt::Horizontal, tr("Fecha/Hora"));
    model->setHeaderData(6, Qt::Horizontal, tr("Datos previos"));
    model->setHeaderData(7, Qt::Horizontal, tr("Datos nuevos"));

    ui->tableView->setModel(model);
    ui->tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableView->hideColumn(0);
    ui->tableView->resizeColumnsToContents();

    ui->tableView->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    ui->tableView->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    ui->tableView->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    ui->tableView->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    ui->tableView->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    ui->tableView->horizontalHeader()->setSectionResizeMode(6, QHeaderView::ResizeToContents);
    ui->tableView->horizontalHeader()->setSectionResizeMode(7, QHeaderView::ResizeToContents);

    ui->tableView->setAlternatingRowColors(true);
    ui->tableView->setStyleSheet(
        "QTableView { alternate-background-color: #f5f5f5; }"
        "QTableView::item { padding: 5px; }"
        );

    for (int col = 2; col <= 7; ++col) {
        ui->tableView->model()->setHeaderData(col, Qt::Horizontal, Qt::AlignCenter, Qt::TextAlignmentRole);
    }
}

void auditoria::connectButtons()
{
    connect(ui->pushBuscar, &QPushButton::clicked, this, &auditoria::onBuscarClicked);
    connect(ui->pushExportar, &QPushButton::clicked, this, &auditoria::onExportarClicked);
    connect(ui->lineEdit, &QLineEdit::textEdited, this, &auditoria::onFiltrarTextoCambiado);
}

void auditoria::onBuscarClicked()
{
    QString filtro = ui->lineEdit->text().trimmed();
    QSqlQueryModel *model = qobject_cast<QSqlQueryModel*>(ui->tableView->model());

    if (!model) return;

    QString queryStr = "SELECT al.id, u.username AS usuario, al.action AS accion, "
                       "al.affected_table AS tabla, al.record_id AS registro, "
                       "al.event_date AS fecha "
                       "FROM audit_logs al "
                       "LEFT JOIN users u ON al.user_id = u.id ";

    if (!filtro.isEmpty()) {
        queryStr += "WHERE u.username LIKE '%" + filtro + "%' OR "
                                                          "al.action LIKE '%" + filtro + "%' OR "
                               "al.affected_table LIKE '%" + filtro + "%' OR "
                               "al.record_id::text LIKE '%" + filtro + "%' ";
    }

    queryStr += "ORDER BY al.event_date DESC";

    model->setQuery(queryStr, dbmanager::instance().getDatabase());
    ui->tableView->resizeColumnsToContents();
}

void auditoria::onFiltrarTextoCambiado(const QString &text)
{
    Q_UNUSED(text);
    onBuscarClicked();
}

void auditoria::onExportarClicked()
{
    exportdialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        QString fileName = QFileDialog::getSaveFileName(
            this,
            "Exportar datos",
            QDir::homePath(),
            getFileFilters(dialog.selectedFormat())
            );

        if (fileName.isEmpty()) {
            return;
        }

        try {
            switch (dialog.selectedFormat()) {
            case exportdialog::CSV:
                exportarCSV(fileName);
                QMessageBox::information(this, "Éxito", "Datos exportados correctamente a CSV");
                break;
            default:
                exportarJSON(fileName);
                QMessageBox::information(this, "Éxito", "Datos exportados correctamente a JSON");
                break;
            }
        } catch (const std::exception& e) {
            QMessageBox::critical(this, "Error", QString("Error al exportar: ") + e.what());
        }
    }
}

void auditoria::exportarJSON(const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        throw std::runtime_error("No se pudo crear el archivo: " + file.errorString().toStdString());
    }

    QSqlQueryModel *model = qobject_cast<QSqlQueryModel*>(ui->tableView->model());
    if (!model) return;

    QJsonArray jsonArray;

    QStringList headers;
    for (int col = 1; col < model->columnCount(); ++col) {
        headers << model->headerData(col, Qt::Horizontal).toString();
    }

    for (int row = 0; row < model->rowCount(); ++row) {
        QJsonObject jsonObj;
        for (int col = 1; col < model->columnCount(); ++col) {
            QModelIndex index = model->index(row, col);
            QString header = headers.at(col-1);
            QString value = model->data(index).toString();

            if (header.compare("Fecha/Hora", Qt::CaseInsensitive) == 0) {
                QDateTime dateTime = QDateTime::fromString(value, "dd/MM/yyyy HH:mm");
                if (dateTime.isValid()) {
                    value = dateTime.toString(Qt::ISODate);
                }
            }

            jsonObj[header] = value;
        }
        jsonArray.append(jsonObj);
    }


    QJsonObject finalJson;
    QJsonDocument doc(finalJson);
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
}

QString auditoria::getFileFilters(exportdialog::ExportFormat format) const
{
    switch (format) {
    case exportdialog::CSV:
        return "Archivos CSV (*.csv)";
    case exportdialog::JSON:
        return "Archivos JSON (*.json)";
    default:
        return "Todos los archivos (*)";
    }
}

void auditoria::exportarCSV(const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        throw std::runtime_error("No se pudo crear el archivo: " + file.errorString().toStdString());
    }

    QTextStream out(&file);
    QSqlQueryModel *model = qobject_cast<QSqlQueryModel*>(ui->tableView->model());
    if (!model) return;

    for (int i = 1; i < model->columnCount(); ++i) {
        out << "\"" << model->headerData(i, Qt::Horizontal).toString() << "\"";
        if (i < model->columnCount() - 1)
            out << ",";
        else
            out << "\n";
    }

    for (int row = 0; row < model->rowCount(); ++row) {
        for (int col = 1; col < model->columnCount(); ++col) {
            QModelIndex index = model->index(row, col);
            out << "\"" << model->data(index).toString().replace("\"", "\"\"") << "\"";
            if (col < model->columnCount() - 1)
                out << ",";
            else
                out << "\n";
        }
    }

    file.close();
}

