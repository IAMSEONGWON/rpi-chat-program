QT       += core gui network multimedia

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    videosender.cpp \
    videoreceiver.cpp

HEADERS += \
    mainwindow.h \
    videosender.h \
    videoreceiver.h

FORMS += \
    mainwindow.ui

# OpenCV 설정 (MinGW 4.5.5)
INCLUDEPATH += C:/opencv_mingw/include
LIBS += -LC:/opencv_mingw/x64/mingw/lib
LIBS += -lopencv_highgui455 \
        -lopencv_videoio455 \
        -lopencv_imgcodecs455 \
        -lopencv_imgproc455 \
        -lopencv_core455
