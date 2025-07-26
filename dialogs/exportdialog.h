#ifndef EXPORTDIALOG_H
#define EXPORTDIALOG_H

#include <QDialog>

namespace Ui {
class exportdialog;
}

class exportdialog : public QDialog
{
    Q_OBJECT

public:
    enum ExportFormat {
        CSV,
        JSON
    };

    explicit exportdialog(QWidget *parent = nullptr);
    ~exportdialog();

    ExportFormat selectedFormat() const;

private:
    Ui::exportdialog *ui;
};

#endif // EXPORTDIALOG_H
