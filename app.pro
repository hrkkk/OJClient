QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    adduserdialog.cpp \
    adminwindow.cpp \
    alterdialog.cpp \
    answerdialog.cpp \
    createexamdialog.cpp \
    examdialog.cpp \
    loginwindow.cpp \
    main.cpp \
    md5.cpp \
    studentwindow.cpp \
    tcpclient.cpp \
    teacherwindow.cpp

HEADERS += \
    adduserdialog.h \
    adminwindow.h \
    alterdialog.h \
    answerdialog.h \
    createexamdialog.h \
    define.h \
    examdialog.h \
    json.hpp \
    loginwindow.h \
    md5.h \
    studentwindow.h \
    tcpclient.h \
    teacherwindow.h

FORMS += \
    adduserdialog.ui \
    adminwindow.ui \
    alterdialog.ui \
    answerdialog.ui \
    createexamdialog.ui \
    examdialog.ui \
    loginwindow.ui \
    studentwindow.ui \
    teacherwindow.ui

RC_ICONS = logo.ico

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    res.qrc
