QT       += core gui sql multimedia

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    dbmanager.cpp \
    dirmonitor.cpp \
    ed2k.cpp \
    file.cpp \
    filesview.cpp \
    filter.cpp \
    filterform.cpp \
    forpostdialog.cpp \
    freenetclipboard.cpp \
    freenetwindow.cpp \
    hash.cpp \
    hashthread.cpp \
    main.cpp \
    mainwindow.cpp \
    trafficlights.cpp

HEADERS += \
    dbmanager.h \
    dirmonitor.h \
    ed2k.h \
    file.h \
    filesview.h \
    filter.h \
    filterform.h \
    forpostdialog.h \
    freenetclipboard.h \
    freenetwindow.h \
    hash.h \
    hashthread.h \
    mainwindow.h \
    todo.h \
    trafficlights.h

FORMS += \
    filterform.ui \
    forpostdialog.ui \
    freenetclipboard.ui \
    freenetwindow.ui \
    mainwindow.ui


# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
RC_ICONS = donkey.ico
RESOURCES += \
    muldup.qrc
