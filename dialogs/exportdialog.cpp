#include "dialogs/exportdialog.h"
#include "ui_exportdialog.h"

exportdialog::exportdialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::exportdialog)
{
    ui->setupUi(this);
    setWindowTitle("Seleccionar formato de exportación");
}

exportdialog::~exportdialog()
{
    delete ui;
}

exportdialog::ExportFormat exportdialog::selectedFormat() const
{
    if (ui->radioButton_2->isChecked()) return CSV;
    if (ui->radioButton_3->isChecked()) return JSON;
}
