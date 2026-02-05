#include "videoreceiver.h"
#include <QDebug>

VideoReceiver::VideoReceiver(QString ip, int port, QString targetNick, QObject *parent)
    : QThread(parent), m_running(true)
{
    // 서버의 웹 스트리밍 주소 완성 (예: http://192.168.0.11:9999/yun)
    m_url = QString("http://%1:%2/%3").arg(ip).arg(port).arg(targetNick);
}

void VideoReceiver::stop()
{
    QMutexLocker locker(&m_mutex);
    m_running = false;
}

void VideoReceiver::run()
{
    // OpenCV는 웹 주소(URL)를 넣으면 자동으로 스트리밍을 가져옴
    cv::VideoCapture cap(m_url.toStdString());

    if (!cap.isOpened()) {
        qDebug() << "Failed to connect stream:" << m_url;
        return;
    }

    cv::Mat frame;

    while (m_running) {
        cap >> frame; // 네트워크로 영상 프레임 한 장 받기
        if (frame.empty()) {
            // 끊기면 잠시 대기 후 재시도 or 종료 (여기선 잠시 대기)
            msleep(100);
            continue;
        }

        // OpenCV(BGR) -> Qt(RGB) 변환
        cv::Mat rgbFrame;
        cv::cvtColor(frame, rgbFrame, cv::COLOR_BGR2RGB);
        QImage img((const uchar*)rgbFrame.data, rgbFrame.cols, rgbFrame.rows, rgbFrame.step, QImage::Format_RGB888);

        // UI로 전송
        emit frameReceived(img.copy());

        // 너무 빠르면 CPU 먹으니까 살짝 쉼
        msleep(10);
    }
    cap.release();
}
