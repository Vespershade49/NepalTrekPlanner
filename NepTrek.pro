QT += widgets
QT += sql
CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    main.cpp \
    mainwindow.cpp \


HEADERS += \
    Routeoptimizer.h \
    backend.h \
    bookingdetail.h \
    budgetcalculator.h \
    data.h \
    emergencyinfo.h \
    filemanager.h \
    guide.h \
    itinerary.h \
    logindialog.h \
    mainwindow.h \
    mysqlmanager.h \
    permit.h \
    review.h \
    touristspot.h \
    trekroute.h \
    userprofile.h \

# NOTE: mainwindow.ui removed on purpose - the UI is built entirely in
# code inside mainwindow.cpp (setupSpotsTab, setupTrekTab, etc.). Having
# both a .ui file AND code-built widgets with the same names causes
# duplicate-definition / "ui_mainwindow.h not found or conflicting"
# build errors. Don't add mainwindow.ui back unless mainwindow.cpp is
# rewritten to use it instead.
#
# NOTE: logindialog.cpp removed on purpose too - LoginDialog is defined
# entirely inside logindialog.h (header-only class), so a separate .cpp
# for it causes duplicate symbol errors at link time. If a
# logindialog.cpp file still exists in this folder, delete it.

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES += \
    .gitignore