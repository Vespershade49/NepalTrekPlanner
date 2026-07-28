QT += widgets
QT += sql
CONFIG += c++17



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


# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES += \
    .gitignore
