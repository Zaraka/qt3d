!include( ../examples.pri ) {
    error( "Couldn't find the examples.pri file!" )
}

QT += 3dcore 3drender 3dinput 3dquick qml quick 3dextras

SOURCES += \
    main.cpp

OTHER_FILES += \
    main.qml

RESOURCES += \
    cylinder-qml.qrc
