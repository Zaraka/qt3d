TARGET = tst_qglindexbuffer
CONFIG += testcase
TEMPLATE=app
QT += testlib 3d opengl
CONFIG += warn_on

SOURCES += tst_qglindexbuffer.cpp
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
