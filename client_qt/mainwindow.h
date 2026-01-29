#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpSocket> // 소켓 통신을 위해 필수

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

private:
    Ui::MainWindow *ui;
    QTcpSocket *socket; // 소켓 객체
};

#endif // MAINWINDOW_H
