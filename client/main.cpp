#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QTcpSocket>
#include <QHostAddress>
#include "../common/defs.h"

class ChatWidget : public QWidget {
public:
    ChatWidget(QWidget *parent = nullptr) : QWidget(parent) {
        // UI 구성
        QVBoxLayout *layout = new QVBoxLayout(this);
        display = new QTextEdit(this);
        display->setReadOnly(true);
        
        input = new QLineEdit(this);
        btnSend = new QPushButton("전송", this);

        layout->addWidget(display);
        layout->addWidget(input);
        layout->addWidget(btnSend);

        // 소켓 설정
        socket = new QTcpSocket(this);

        // 이벤트 연결
        // 서버 연결 (본인 환경에 맞게 IP 수정)
        socket->connectToHost(QHostAddress("192.168.0.20"), PORT_NUM); 

        // 데이터 수신
        connect(socket, &QTcpSocket::readyRead, this, [=](){
            QByteArray data = socket->readAll();
            display->append("Server: " + QString::fromUtf8(data));
        });

        // 전송 버튼 클릭
        connect(btnSend, &QPushButton::clicked, this, &ChatWidget::sendMessage);
        
        // 엔터키 입력 시 전송
        connect(input, &QLineEdit::returnPressed, this, &ChatWidget::sendMessage);
        
        display->append("서버 연결 시도 중...");
    }

private:
    QTextEdit *display;
    QLineEdit *input;
    QPushButton *btnSend;
    QTcpSocket *socket;

    void sendMessage() {
        if(socket->state() == QAbstractSocket::ConnectedState) {
            QByteArray data = input->text().toUtf8();
            socket->write(data);
            display->append("나: " + input->text()); // 클라이언트 화면에 표시
            input->clear();
        } else {
            display->append("서버와 연결되지 않았습니다.");
        }
    }
};

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    ChatWidget w;
    w.resize(400, 300);
    w.show();
    return a.exec();
}