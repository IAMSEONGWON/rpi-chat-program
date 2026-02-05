#include "videosender.h"
#include <QDebug>

VideoSender::VideoSender(QString ip, int port, QString nickname, QObject *parent)
    : QThread(parent), m_ip(ip), m_port(port), m_nickname(nickname), m_running(true)
{
}

void VideoSender::stop()
{
    QMutexLocker locker(&m_mutex);
    m_running = false;
}

void VideoSender::run()
{
    // 영상 소켓 생성 및 연결
    QTcpSocket socket;
    socket.connectToHost(m_ip, m_port);

    if (!socket.waitForConnected(3000)) {
        qDebug() << "Video Server Connection Failed!";
        return;
    }

    // 프로토콜 전송: "UPLOAD 닉네임\n" (서버 규칙 준수)
    QString cmd = QString("UPLOAD %1\n").arg(m_nickname);
    socket.write(cmd.toUtf8());
    socket.waitForBytesWritten();

    // 카메라 열기 (0번: 기본 웹캠)
    cv::VideoCapture cap(0, cv::CAP_DSHOW);
    if (!cap.isOpened()) {
        // 혹시 0번이 안 되면 1번 시도
        cap.open(1, cv::CAP_DSHOW);
    }
    cap.set(cv::CAP_PROP_FRAME_WIDTH, 320);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, 240);
    cap.set(cv::CAP_PROP_FPS, 30);

    if (!cap.isOpened()) {
        qDebug() << "Camera Open Failed!";
        socket.close();
        return;
    }

    cv::Mat frame;
    std::vector<uchar> buffer;
    std::vector<int> params = {cv::IMWRITE_JPEG_QUALITY, 50};

    while (m_running) {
        cap >> frame;
        if (frame.empty()) continue;

        // [UI 표시용] OpenCV(BGR) -> Qt(RGB) 변환
        cv::Mat rgbFrame;
        cv::cvtColor(frame, rgbFrame, cv::COLOR_BGR2RGB);
        QImage img((const uchar*)rgbFrame.data, rgbFrame.cols, rgbFrame.rows, rgbFrame.step, QImage::Format_RGB888);
        emit frameCaptured(img.copy()); // 메인 윈도우로 전송

        // [서버 전송용] JPG 인코딩
        cv::imencode(".jpg", frame, buffer, params);

        // 데이터 크기 (4byte) + 데이터 전송
        int imgSize = (int)buffer.size();
        socket.write((const char*)&imgSize, sizeof(int));
        socket.write((const char*)buffer.data(), imgSize);
        socket.waitForBytesWritten();

        msleep(33); // 30 FPS
    }

    cap.release();
    socket.close();
}
