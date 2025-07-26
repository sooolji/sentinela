#ifndef DASHBOARD_H
#define DASHBOARD_H

#include <QWidget>
#include <QtCharts>
#include <QTimer>
#include "database/dbmanager.h"

namespace Ui {
class dashboard;
}

class dashboard : public QWidget
{
    Q_OBJECT

public:
    explicit dashboard(QWidget *parent = nullptr);
    void refreshData();
    ~dashboard();

private slots:
    void onRefreshTimer();

private:
    void loadMetrics();
    void createRiskChart();
    void loadVulnerableAssets();
    void loadCommonThreats();

    Ui::dashboard *ui;
    QTimer *refreshTimer;
};

#endif // DASHBOARD_H
