#include "dashboard.h"
#include "ui_dashboard.h"
#include "database/dbmanager.h"
#include <QSqlQuery>
#include <QMessageBox>
#include <QPainter>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QValueAxis>
#include <QtCharts/QBarCategoryAxis>
#include <QLegend>

dashboard::dashboard(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::dashboard)
{
    ui->setupUi(this);


    dbmanager& dbManager = dbmanager::instance();
    if(dbManager.isOpen()) {
        refreshData();
    } else {
        QMessageBox::warning(this, "Error", "No se pudo conectar a la base de datos");
    }
}

void dashboard::onRefreshTimer()
{
    if (isVisible()) {
        refreshData();
    }
}

void dashboard::refreshData()
{
    dbmanager& dbManager = dbmanager::instance();

    if (!dbManager.isOpen()) {
        if (!dbManager.getDatabase().open()) {
            QMessageBox::warning(this, "Error", "No se pudo conectar a la base de datos");
            return;
        }
    }

    if(dbManager.isOpen()) {
        loadMetrics();
        createRiskChart();
        loadVulnerableAssets();
        loadCommonThreats();
    }
}

void dashboard::createRiskChart()
{
    dbmanager& dbManager = dbmanager::instance();

    QChart *chart = new QChart();
    chart->setTitle("Sistema Sentinela - Gestión de Riesgos");
    chart->setAnimationOptions(QChart::SeriesAnimations);
    chart->setTheme(QChart::ChartThemeLight);

    chart->setBackgroundBrush(QBrush(Qt::white));
    chart->setBackgroundRoundness(15);
    chart->setMargins(QMargins(15, 15, 15, 15));

    QFont titleFont("Segoe UI", 14, QFont::Bold);
    QFont labelFont("Segoe UI", 9);
    QFont axisTitleFont("Segoe UI", 10, QFont::Medium);

    chart->setTitleFont(titleFont);
    chart->legend()->setVisible(true);
    chart->legend()->setAlignment(Qt::AlignBottom);
    chart->legend()->setFont(labelFont);
    chart->legend()->setBackgroundVisible(false);
    chart->legend()->setBorderColor(Qt::transparent);

    QMap<QString, QColor> typeColors = {
        {"HW", QColor(44, 123, 182)},   // Azul oscuro
        {"SW", QColor(171, 217, 233)},  // Azul claro
        {"DT", QColor(255, 127, 14)},   // Naranja
        {"PR", QColor(89, 161, 79)},    // Verde
        {"DO", QColor(197, 90, 17)}     // Rojo oscuro
    };

    QSqlQuery query(dbManager.getDatabase());
    query.prepare("SELECT a.type, AVG(r.risk_weight) "
                  "FROM risks r JOIN assets a ON r.asset_id = a.id "
                  "GROUP BY a.type ORDER BY a.type");

    if(!query.exec()) {
        qWarning() << "Error en consulta:" << query.lastError().text();
        return;
    }

    QBarSeries *series = new QBarSeries();
    QStringList categories;
    double maxRisk = 0.0;

    while(query.next()) {
        QString type = query.value(0).toString();
        double risk = query.value(1).toDouble();

        if(risk > maxRisk) maxRisk = risk;

        QString typeName;
        if(type == "HW") typeName = "Hardware";
        else if(type == "SW") typeName = "Software";
        else if(type == "DT") typeName = "Datos";
        else if(type == "PR") typeName = "Procesos";
        else typeName = "Documentos";

        QBarSet *set = new QBarSet(typeName);
        *set << risk;

        // Aplicar color y estilo
        set->setColor(typeColors.value(type));
        set->setBorderColor(QColor(80, 80, 80, 150));
        set->setLabelColor(Qt::black);
        set->setLabelFont(labelFont);

        series->append(set);
        categories << typeName;
    }

    series->setBarWidth(0.80);
    series->setLabelsVisible(false);
    series->setLabelsFormat("@value");
    series->setLabelsPosition(QAbstractBarSeries::LabelsOutsideEnd);
    series->setLabelsPrecision(8); //6

    chart->addSeries(series);

    QBarCategoryAxis *axisX = new QBarCategoryAxis();
    axisX->append(categories);
    //axisX->setTitleText("Tipos de Activos");
    axisX->setTitleFont(axisTitleFont);
    axisX->setLabelsFont(labelFont);
    axisX->setGridLineColor(QColor(220, 220, 220));
    //chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    QValueAxis *axisY = new QValueAxis();
    double upperBound = (maxRisk < 0.1) ? 0.1 : (maxRisk * 1.10); // 15% de margen
    axisY->setRange(0, upperBound);
    axisY->setTitleText("Riesgo Promedio por Tipo de Activo");
    axisY->setTitleFont(axisTitleFont);
    axisY->setLabelFormat("%.2f");
    axisY->setLabelsFont(labelFont);
    axisY->setTickCount(6);
    axisY->setMinorTickCount(1);
    axisY->setGridLineColor(QColor(220, 220, 220));
    axisY->setMinorGridLineColor(QColor(240, 240, 240));
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);

    ui->chartView->setChart(chart);
    ui->chartView->setRenderHint(QPainter::Antialiasing, true);
    ui->chartView->setRenderHint(QPainter::TextAntialiasing, true);
    ui->chartView->setRubberBand(QChartView::RectangleRubberBand);

    ui->chartView->setBackgroundBrush(QBrush(Qt::white));

    connect(series, &QBarSeries::hovered, this, [](bool status, int index, QBarSet *set) {
        if (status) {
            QToolTip::showText(QCursor::pos(),
                               QString("<div style='background-color: #ffffff; color: #333333; padding: 5px; border: 1px solid #cccccc; border-radius: 3px;'>"
                                       "<b style='color: %1;'>%2</b><br>"
                                       "Riesgo promedio: <b>%3</b>"
                                       "</div>")
                                   .arg(set->color().name())
                                   .arg(set->label())
                                   .arg(set->at(index), 0, 'f', 2),
                               nullptr, QRect(), 3000);
        } else {
            QToolTip::hideText();
        }
    });
}

void dashboard::loadMetrics()
{
    dbmanager& dbManager = dbmanager::instance();

    QSqlQuery threatsQuery(dbManager.getDatabase());
    threatsQuery.prepare("SELECT COUNT(*) FROM threats");
    if(threatsQuery.exec() && threatsQuery.next()) {
        ui->labelThreatsValue->setText(QString::number(threatsQuery.value(0).toInt()));
    } else {
        ui->labelThreatsValue->setText("N/A");
    }

    QSqlQuery assetsQuery(dbManager.getDatabase());
    assetsQuery.prepare("SELECT COUNT(*) FROM assets");
    if(assetsQuery.exec() && assetsQuery.next()) {
        ui->labelAssetsValue->setText(QString::number(assetsQuery.value(0).toInt()));
    } else {
        ui->labelAssetsValue->setText("N/A");
    }

    QSqlQuery riskQuery(dbManager.getDatabase());
    riskQuery.prepare("SELECT system_total_risk FROM total_risk LIMIT 1");
    if(riskQuery.exec() && riskQuery.next()) {
        double risk = riskQuery.value(0).toDouble();
        ui->labelRiskValue->setText(QString("%1%").arg(risk * 100, 0, 'f', 2));

        QString riskColor;
        if(risk > 0.7) riskColor = "#e74c3c";
        else if(risk > 0.4) riskColor = "#f39c12";
        else riskColor = "#27ae60";

        ui->labelRiskValue->setStyleSheet(QString("QLabel { color: %1; }").arg(riskColor));
    } else {
        ui->labelRiskValue->setText("N/A");
        ui->labelRiskValue->setStyleSheet("QLabel { color: #7f8c8d; }");
    }
}

void dashboard::loadVulnerableAssets()
{
    dbmanager& dbManager = dbmanager::instance();
    ui->listVulnerableAssets->clear();

    QSqlQuery query(dbManager.getDatabase());
    query.prepare("SELECT a.description, r.risk_weight "
                  "FROM risks r JOIN assets a ON r.asset_id = a.id "
                  "ORDER BY r.risk_weight DESC LIMIT 5");

    if(query.exec()) {
        while(query.next()) {
            QString asset = query.value(0).toString();
            double risk = query.value(1).toDouble();
            QString itemText = QString("%1 (Riesgo: %2)")
                                   .arg(asset)
                                   .arg(risk, 0, 'f', 2);

            QListWidgetItem *item = new QListWidgetItem(itemText);
            ui->listVulnerableAssets->addItem(item);
        }
    }
}

void dashboard::loadCommonThreats()
{
    dbmanager& dbManager = dbmanager::instance();
    ui->listCommonThreats->clear();

    QSqlQuery query(dbManager.getDatabase());
    query.prepare("SELECT t.description, COUNT(at.threat_id) as count "
                  "FROM assets_threats at JOIN threats t ON at.threat_id = t.id "
                  "GROUP BY t.description ORDER BY count DESC LIMIT 5");

    if(query.exec()) {
        while(query.next()) {
            QString threat = query.value(0).toString();
            int count = query.value(1).toInt();
            QString itemText = QString("%1 (%2 activos afectados)")
                                   .arg(threat)
                                   .arg(count);

            QListWidgetItem *item = new QListWidgetItem(itemText);
            ui->listCommonThreats->addItem(item);
        }
    }
}

dashboard::~dashboard()
{
    refreshTimer->stop();
    delete refreshTimer;
    delete ui;
}
