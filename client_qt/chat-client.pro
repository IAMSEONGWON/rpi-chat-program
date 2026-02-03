QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

SOURCES += \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    mainwindow.h

FORMS += \
    mainwindow.ui

# OpenCV 설정 (MinGW 4.5.5)

# 헤더 파일 경로
INCLUDEPATH += C:/opencv_mingw/include

# 라이브러리 경로
LIBS += -LC:/opencv_mingw/x64/mingw/lib

# 필요한 라이브러리 파일들을 개별적으로 연결
LIBS += -lopencv_highgui455 \
        -lopencv_videoio455 \
        -lopencv_imgcodecs455 \
        -lopencv_imgproc455 \
        -lopencv_core455
