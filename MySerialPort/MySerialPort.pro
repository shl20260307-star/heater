TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
QT+= widgets
SOURCES += main.cpp \
    widget.cpp \
    wellplatedialog.cpp \
    qcustomplot.cpp
QT      += serialport

FORMS += \
    widget.ui

HEADERS += \
    widget.h \
    wellplatedialog.h \
    qcustomplot.h
QT      += printsupport



