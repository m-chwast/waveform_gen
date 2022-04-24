QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    serial_class.cpp \
    serialib.cpp \
    wave.cpp \
    wave_data.cpp \
    waveeditform.cpp

HEADERS += \
    mainwindow.h \
    serial_class.h \
    serialib.h \
    wave.h \
    wave_data.h \
    waveeditform.h

FORMS += \
    mainwindow.ui \
    waveeditform.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES += \
    sinx.png

RESOURCES += \
    resource1.qrc
