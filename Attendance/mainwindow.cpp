#include <iostream>
#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QIntValidator>
#include <QPixmap>
#include <QString>
#include <QDir>
#include <QFileInfo>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>

#include "user.h"
#include "attendance.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->lnEdtMemberNo->setValidator(new QIntValidator(0, 99999, this));

    mImagesDir      = QDir::currentPath() + QString("\\..\\Images\\");
    mpHttpMgr       = new QNetworkAccessManager();
    mpStatusTimer   = new QTimer();
    connect(mpStatusTimer, SIGNAL(timeout()), this, SLOT(clearStatus()));

    setPhoto("logo_01.jpg");
    setWindowIcon(QIcon(mImagesDir + "icon_attendance.jpg"));
}

MainWindow::~MainWindow() {
    delete ui;
}


// ----------------------------------------------------------
//                   Network Requests & Responses
//                  142.93.216.207,  192.168.83.12
// ----------------------------------------------------------
void MainWindow::on_lnEdtMemberNo_returnPressed()
{
    QNetworkRequest request;
    QString userEnteredId   = ui->lnEdtMemberNo->text();
    QString strUrl          = QString("http://142.93.216.207:8080/user/") + userEnteredId;
    //QString strUrl          = QString("http://142.93.216.207:8080/user/") + userEnteredId;
    request.setUrl(QUrl(strUrl));

    connect(mpHttpMgr,&QNetworkAccessManager::finished, this, &MainWindow::onUserResponse);
    mpHttpMgr->get(request);
}

void MainWindow::onUserResponse(QNetworkReply *pReply) {
    disconnect(mpHttpMgr,&QNetworkAccessManager::finished, this, &MainWindow::onUserResponse);

    json root;
    const auto& resp= QString(pReply->readAll());

    mpUser          = User::parseUser(resp);
    if(mpUser) {
        QNetworkRequest request;
        //request.setUrl(QUrl("http://142.93.216.207:8080/attendance"));
        request.setUrl(QUrl("http://142.93.216.207:8080/attendance"));

        root["id"]      = mpUser->getMembershipNo();
        QString jsPkt   = QString(root.dump().c_str());

        connect(mpHttpMgr,&QNetworkAccessManager::finished, this, &MainWindow::onAttendanceMarked);
        mpHttpMgr->put(request, jsPkt.toUtf8());
    } else updateStatus("Member not found");
}

void MainWindow::onAttendanceMarked(QNetworkReply *pReply) {
    disconnect(mpHttpMgr,&QNetworkAccessManager::finished, this, &MainWindow::onAttendanceMarked);

    ui->lblName->setText(mpUser->getName());
    QString strPhoto    = getPhoto(mpUser->getMembershipNo());
    setPhoto(strPhoto);

    const auto& resp            = QString(pReply->readAll());
    Attendance::Ptr pAttendance = Attendance::parseAttendance(resp);
    if(!pAttendance) { updateStatus("Attendace already marked"); return; }

    QString strDisplay;
    int32_t duration    = pAttendance->getDuration();
    if(duration == 0) {
        strDisplay  = QString("Your subscription expires on ") + mpUser->getValidityEnd();
    } else {
        int32_t hh  = duration / 3600;
        duration    = duration % 3600;
        int32_t mm  = duration / 60;
        strDisplay  = "You workedout for "
                        + QString::number(hh)
                        + QString(":")
                        + QString::number(mm) + "(HH:MM), Have a Nice Day.";
    }
    updateStatus(strDisplay);
}

int32_t MainWindow::getMembershipNoFromFileName(QString pFileName) {
    QString strNo;
    if(pFileName.isEmpty()) return 0;
    for(const auto& ch : pFileName) if(ch.isDigit()) strNo += ch;
    if(strNo.isEmpty()) return 0;
    return strNo.toInt();
}

QString MainWindow::getPhoto(int32_t pMembershipNo) {
    QString userPic;
    QDir imageDir       = mImagesDir;
    QStringList allFiles= imageDir.entryList(QStringList() << "*.jpg" << "*.JPG" << "*.jpeg" << "*.JPEG" << "*.png" << "*.PNG");

    int32_t memberNo    = pMembershipNo;
    for(const auto& imageFile : allFiles)
        if(getMembershipNoFromFileName(imageFile) == memberNo) {
            userPic = imageFile;
            break;
        }
    return userPic;
}

void MainWindow::setPhoto(QString pPhotoFile) {
    if(pPhotoFile.isEmpty()) return;

    QFileInfo checkFile(mImagesDir + pPhotoFile);
    if(!checkFile.exists() || !checkFile.isFile()) pPhotoFile = "logo_01.jpg";

    QPixmap userImage(mImagesDir + pPhotoFile);
    ui->lblLogo->setPixmap(userImage.scaled(300,300));
    ui->lblLogo->setAlignment(Qt::AlignmentFlag::AlignCenter);
}

// ----------------------------------------------------------
//                   Update and Clear Status
// ----------------------------------------------------------

void MainWindow::updateStatus(QString pStatus, int delay) {
    mpStatusTimer->stop();
    ui->lblSubscriptionDetails->setText(pStatus);
    mpStatusTimer->start(delay);
}

void MainWindow::clearStatus() {
    mpStatusTimer->stop();
    ui->lblSubscriptionDetails->clear();
    ui->lblName->clear();
    ui->lnEdtMemberNo->clear();

    setPhoto("logo_01.jpg");
}



/*
#include <QtNetwork/QNetworkDatagram>
#include <QFile>
#include <QtNetwork/QUdpSocket>
QUdpSocket *pUdpSock;

slot:
    void readDatagrams();

in Constructor: pUdpSock(nullptr)

//Create UDP Socket
    pUdpSock = new QUdpSocket();
    pUdpSock->bind(QHostAddress::LocalHost, 50001);
    connect(pUdpSock, &QUdpSocket::readyRead, this, &MainWindow::readDatagrams);

//	Connect signal and slot
    QObject::connect(pHttpMgr, &QNetworkAccessManager::finished,
            this, [=](QNetworkReply *reply) {
            if (reply->error()) { qDebug() << reply->errorString(); return; }
                    QString answer = reply->readAll();
                    qDebug() << answer;
                }
        );

void MainWindow::readDatagrams() {
    std::cout << "Inside readDatagram" << std::endl;
    QFile fp("photo.txt");
    fp.open(QIODevice::WriteOnly | QIODevice::Text);

    QTextStream fpOut(&fp);
    while (pUdpSock->hasPendingDatagrams()) {
        QNetworkDatagram datagram = pUdpSock->receiveDatagram();
        auto pPkt = datagram.data();
        fpOut << pPkt;
    }
    fpOut.flush();
    fp.close();
}
*/
