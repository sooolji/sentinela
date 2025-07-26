#include "sentinela.h"
#include "forms/login.h"
#include "database/dbmanager.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    dbmanager::instance("sentinela");

    login loginDialog;
    if(loginDialog.exec() != QDialog::Accepted) {
        return 0;
    }

    dbmanager::instance().setCurrentUserId(loginDialog.getUserId());

    sentinela w(nullptr, loginDialog.getUserId(), loginDialog.getIsAdmin());
    w.show();
    return a.exec();
}



