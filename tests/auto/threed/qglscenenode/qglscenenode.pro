TARGET = tst_qglscenenode
CONFIG += testcase
TEMPLATE=app
QT += testlib 3d
CONFIG += warn_on

INCLUDEPATH += ../../../shared
SOURCES += tst_qglscenenode.cpp
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
