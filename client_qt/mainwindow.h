#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpSocket> // 소켓 통신을 위해 필수
#include "videosender.h" // 영상 스레드 헤더
#include "videoreceiver.h" // 영상 수신 헤더

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // 버튼 클릭이나 데이터 수신 시 실행될 함수들 (Slot)
    void connectToServer();
    void sendMessage();
    void receiveMessage();

    // 영상 스레드에서 온 이미지를 화면에 갱신
    void updateVideoUI(QImage img);
    void onBtnWatchClicked(); // 보기 버튼 슬롯
    void updateOpponentUI(QImage img); // 상대방 화면 갱신

private:
    Ui::MainWindow *ui;
    QTcpSocket *socket; // 소켓 객체
    VideoSender *videoSender = nullptr; // 영상 송출 객체
    VideoReceiver *videoReceiver = nullptr; // [추가] 수신 객체
};

#endif // MAINWINDOW_H
