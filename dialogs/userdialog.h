#ifndef USERDIALOG_H
#define USERDIALOG_H

#include <QDialog>
#include <QMap>
#include <QVariant>

namespace Ui {
class UserDialog;
}

class UserDialog : public QDialog
{
    Q_OBJECT

public:
    explicit UserDialog(QWidget *parent = nullptr, bool isEditMode = false);
    ~UserDialog();

    void setUserData(const QMap<QString, QVariant> &data);
    QMap<QString, QVariant> getUserData() const;

private slots:
    void on_showPasswordCheckBox_stateChanged(Qt::CheckState state);


private:
    Ui::UserDialog *ui;
    bool m_isEditMode;
};

#endif // USERDIALOG_H
