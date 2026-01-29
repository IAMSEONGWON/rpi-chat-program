#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QHostAddress> // IP 주소 처리를 위함

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // 소켓 생성
    socket = new QTcpSocket(this);

    // 시그널-슬롯 연결 (이벤트 연결)

    // "연결" 버튼 누르면 -> connectToServer 함수 실행
    connect(ui->btnConnect, &QPushButton::clicked, this, &MainWindow::connectToServer);

    // "전송" 버튼 누르면 -> sendMessage 함수 실행
    connect(ui->btnSend, &QPushButton::clicked, this, &MainWindow::sendMessage);

    // 입력창에서 엔터 쳐도 -> sendMessage 함수 실행
    connect(ui->lineEdit, &QLineEdit::returnPressed, this, &MainWindow::sendMessage);

    // 소켓에 데이터가 들어오면 -> receiveMessage 함수 실행
    connect(socket, &QTcpSocket::readyRead, this, &MainWindow::receiveMessage);
}

MainWindow::~MainWindow()
{
    delete ui;
}

// 기능 구현

void MainWindow::connectToServer()
{
    socket->connectToHost("192.168.0.20", 12345); // IP 수정 필요

    // 서버에 연결되면 닉네임 칸 수정 불가능 (Read Only) + 연결 버튼 비활성화
    ui->leNickname->setReadOnly(true);
    ui->btnConnect->setEnabled(false);

    ui->textLog->append("서버 접속 시도 중...");
}

void MainWindow::sendMessage()
{
    if(socket->state() == QAbstractSocket::ConnectedState) {
        // 닉네임과 메시지 불러오기
        QString nickname = ui->leNickname->text();
        QString message = ui->lineEdit->text();

        // 닉네임이 비어있으면 기본값 설정
        if(nickname.isEmpty()) nickname = "Unknown";

        // 콘솔 버전과 똑같은 형식으로 합치기: "[닉네임] 메시지"
        QString packet = QString("[%1] %2").arg(nickname, message);

        // 서버로 전송 (UTF-8 변환)
        socket->write(packet.toUtf8());

        // 내 화면(로그)에도 표시
        ui->textLog->append(packet);

        // 입력창 비우기
        ui->lineEdit->clear();
    } else {
        ui->textLog->append("먼저 서버에 연결해주세요.");
    }
}

void MainWindow::receiveMessage()
{
    // 서버에서 온 데이터를 모두 읽음
    QByteArray data = socket->readAll();
    // 화면에 출력
    ui->textLog->append(QString::fromUtf8(data));
}
