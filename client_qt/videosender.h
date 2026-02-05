#ifndef VIDEOSENDER_H
#define VIDEOSENDER_H

#include <QThread>
#include <QTcpSocket>
#include <QImage>
#include <QMutex>
#include <opencv2/opencv.hpp>

class VideoSender : public QThread
{
    Q_OBJECT

public:
    explicit VideoSender(QString ip, int port, QString nickname, QObject *parent = nullptr);
    void stop();

signals:
    // UI에 이미지를 보여주기 위한 신호
    void frameCaptured(QImage img);

protected:
    void run() override;

private:
    QString m_ip;
    int m_port;
    QString m_nickname;
    bool m_running;
    QMutex m_mutex;
};

#endif // VIDEOSENDER_H
