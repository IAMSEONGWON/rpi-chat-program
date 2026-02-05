#include <QHostAddress> // IP 주소 처리를 위함
#include <QPixmap> // 이미지 표시용
#include <opencv2/opencv.hpp>
#include "mainwindow.h"
#include "ui_mainwindow.h"

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

    // [추가] "보기" 버튼 누르면 -> onBtnWatchClicked 함수 실행
    // (ui 파일에 btnWatch 버튼이 있어야 함)
    if(ui->btnWatch) {
        connect(ui->btnWatch, &QPushButton::clicked, this, &MainWindow::onBtnWatchClicked);
    }

    cv::Mat testMat = cv::Mat::zeros(100, 100, CV_8UC3);
    qDebug() << "OpenCV Version:" << CV_VERSION;
}

MainWindow::~MainWindow()
{
    // 종료 시 영상 스레드 정리
    if (videoSender != nullptr) {
        videoSender->stop();
        videoSender->wait();
        delete videoSender;
    }

    // [추가] 수신 스레드 정리
    if (videoReceiver != nullptr) {
        videoReceiver->stop();
        videoReceiver->wait();
        delete videoReceiver;
    }

    delete ui;
}

// 기능 구현

void MainWindow::connectToServer()
{
    // UI에서 닉네임 가져오기
    QString nickname = ui->leNickname->text();
    if(nickname.isEmpty()) nickname = "QtUser"; // 닉네임 없으면 기본값

    // 서버 연결 시도 (IP로 수정 필요) - 채팅 서버 (포트 12345)
    socket->connectToHost("192.168.0.11", 12345);

    // 연결 시도 직후, 닉네임 패킷 먼저 전송
    // Qt 소켓은 비동기라 연결되기 전이라도 버퍼에 담아뒀다가 연결 즉시 보냄
    socket->write(nickname.toUtf8());

    // UI 상태 변경 (연결 중임을 표시)
    ui->textLog->append("서버에 접속을 시도 중...");

    // 서버에 연결되면 닉네임 칸 수정 불가능 (Read Only) + 연결 버튼 비활성화
    ui->leNickname->setReadOnly(true);
    ui->btnConnect->setEnabled(false);

    // ---------------------------------------------------------
    // [추가] 영상 송출 스레드 시작 (포트 9999)
    // ---------------------------------------------------------
    if (videoSender != nullptr) {
        delete videoSender;
    }
    // 라즈베리파이 IP, 포트 9999, 닉네임 전달
    videoSender = new VideoSender("192.168.0.11", 9999, nickname, this);

    // 스레드가 이미지를 보내면 -> UI 갱신 함수 실행
    connect(videoSender, &VideoSender::frameCaptured, this, &MainWindow::updateVideoUI);

    videoSender->start();
    // ---------------------------------------------------------
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

// 영상 UI 업데이트 (내 화면)
void MainWindow::updateVideoUI(QImage img)
{
    // lblVideo 라벨에 이미지 표시 (비율 유지하며 크기 맞춤)
    // ui->lblVideo가 없으면 ui 파일에서 추가해야 함
    if(ui->lblVideo) {
        ui->lblVideo->setPixmap(QPixmap::fromImage(img).scaled(ui->lblVideo->size(), Qt::KeepAspectRatio));
    }
}

// 상대방 영상 보기 버튼 클릭 시
void MainWindow::onBtnWatchClicked()
{
    // ui->leTarget이 없으면 ui 파일에서 추가해야 함 (상대 닉네임 입력칸)
    if(!ui->leTarget) return;

    QString target = ui->leTarget->text();
    if (target.isEmpty()) {
        ui->textLog->append("보고 싶은 상대의 닉네임을 입력하세요.");
        return;
    }

    // 기존에 보고 있던 게 있으면 종료
    if (videoReceiver != nullptr) {
        videoReceiver->stop();
        videoReceiver->wait();
        delete videoReceiver;
    }

    // 수신 스레드 생성 (서버 IP, 포트 9999, 상대방 닉네임)
    videoReceiver = new VideoReceiver("192.168.0.11", 9999, target, this);

    // 이미지 들어오면 화면 갱신 연결
    connect(videoReceiver, &VideoReceiver::frameReceived, this, &MainWindow::updateOpponentUI);

    videoReceiver->start();
    ui->textLog->append("영상 수신 시작: " + target);
}

// 상대방 영상 UI 업데이트
void MainWindow::updateOpponentUI(QImage img)
{
    // ui->lblOpponent가 없으면 ui 파일에서 추가해야 함 (상대 화면 표시용 라벨)
    if (ui->lblOpponent) {
        ui->lblOpponent->setPixmap(QPixmap::fromImage(img).scaled(ui->lblOpponent->size(), Qt::KeepAspectRatio));
    }
}
