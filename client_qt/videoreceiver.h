#ifndef VIDEORECEIVER_H
#define VIDEORECEIVER_H

#include <QThread>
#include <QImage>
#include <QMutex>
#include <opencv2/opencv.hpp>

class VideoReceiver : public QThread
{
    Q_OBJECT
public:
    // 접속할 서버 IP, 포트, 그리고 "보고 싶은 상대방 닉네임"
    explicit VideoReceiver(QString ip, int port, QString targetNick, QObject *parent = nullptr);
    void stop();

signals:
    void frameReceived(QImage img); // 받은 이미지를 UI로 보냄

protected:
    void run() override;

private:
    QString m_url;
    bool m_running;
    QMutex m_mutex;
};

#endif // VIDEORECEIVER_H
