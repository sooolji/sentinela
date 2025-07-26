QT       += core gui widgets sql charts printsupport network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    database/dbmanager.cpp \
    dialogs/exportdialog.cpp \
    dialogs/userdialog.cpp \
    forms/login.cpp \
    main.cpp \
    sentinela.cpp \
    windows/activos.cpp \
    windows/amenaza.cpp \
    windows/auditoria.cpp \
    windows/dashboard.cpp \
    windows/impacto.cpp \
    windows/usuarios.cpp

HEADERS += \
    database/dbmanager.h \
    dialogs/exportdialog.h \
    dialogs/userdialog.h \
    forms/login.h \
    sentinela.h \
    windows/activos.h \
    windows/amenaza.h \
    windows/auditoria.h \
    windows/dashboard.h \
    windows/impacto.h \
    windows/usuarios.h


FORMS += \
    dialogs/exportdialog.ui \
    forms/login.ui \
    sentinela.ui \
    windows/activos.ui \
    windows/amenaza.ui \
    windows/auditoria.ui \
    windows/dashboard.ui \
    windows/impacto.ui \
    dialogs/userdialog.ui \
    windows/usuarios.ui


# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
