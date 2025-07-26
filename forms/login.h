#ifndef LOGIN_H
#define LOGIN_H

#include <QDialog>
#include "database/dbmanager.h"

namespace Ui {
class login;
}

class login : public QDialog
{
    Q_OBJECT

public:
    explicit login(QWidget *parent = nullptr);
    ~login();

    int getUserId() const { return m_userId; }
    bool getIsAdmin() const { return m_isAdmin; }

private slots:
    void on_loginButton_clicked();
    void on_showPasswordCheckBox_stateChanged(Qt::CheckState state);

private:
    Ui::login *ui;
    int m_userId = -1;
    bool m_isAdmin = false;
};

#endif // LOGIN_H
