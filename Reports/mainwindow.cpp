//sgn
#include <QVector>
#include <QDir>
#include <QFileInfo>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include "json.hpp"

using namespace nlohmann;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    mImagesDir      = QDir::currentPath() + QString("\\..\\Images\\");
    setPhoto("logo_01.jpg");
    setWindowIcon(QIcon(mImagesDir + "icon_reports.jpg"));

    mpHttpMgr       = new QNetworkAccessManager();
    mpStatusTimer   = new QTimer();
    connect(mpStatusTimer, SIGNAL(timeout()), this, SLOT(clearStatus()));

    QNetworkRequest request;
    QString strUrl= QString("http://142.93.216.207:8080/epoch");
    request.setUrl(QUrl(strUrl));

    connect(mpHttpMgr,&QNetworkAccessManager::finished, this, &MainWindow::onEpochResponse);
    mpHttpMgr->get(request);
}

MainWindow::~MainWindow()
{
    delete ui;
}


// ----------------------------------------------------------
//                   Network Requests & Responses
//                  142.93.216.207,  192.168.83.12
// ----------------------------------------------------------

void MainWindow::onEpochResponse(QNetworkReply *pReply) {
    disconnect(mpHttpMgr,&QNetworkAccessManager::finished, this, &MainWindow::onEpochResponse);
    QString strResp = pReply->readAll();

    json pRoot  = json::parse(strResp.toStdString(), nullptr, false);
    if(pRoot.is_discarded()) { mCurEpoch = 0; return; }
    if(pRoot.value<bool>("isOk", false))
        mCurEpoch   = pRoot.value<time_t>("epoch", 0);
    qDebug() << "Current Epoch " << mCurEpoch;
}

void MainWindow::on_btnRenewals_clicked() {
    QString strQuery    =   QString("SELECT * FROM user WHERE validity_end < ")
                            + QString::number(mCurEpoch - User::SECS_IN_A_DAY)
                            + QString(" ORDER BY validity_end DESC;");
    queryDB(strQuery);
}

void MainWindow::on_btnAbsentees_clicked() {
    int32_t iTemp = 5 * User::SECS_IN_A_DAY;
    QString strQuery= QString("SELECT * FROM user WHERE validity_end > ")
                + QString::number(mCurEpoch)
                + QString(" AND last_visit < ")
                + QString::number(mCurEpoch - iTemp)
                + QString(" ORDER BY last_visit DESC;");
    queryDB(strQuery);
}

void MainWindow::on_btnBDay_clicked() {
    QString strTemp     = QDate::currentDate().toString("dd-MM");
    QString strQuery    = QString("SELECT * FROM user WHERE ddMM = \"") + strTemp + QString("\";");
    queryDB(strQuery);
}

void MainWindow::on_btnQuery_clicked() {
    QString strQuery    = ui->lnEdtQuery->text();
    if(strQuery.isEmpty()) { updateStatus("Pls Enter a Query"); return; }
    queryDB(strQuery);
}

void MainWindow::queryDB(QString pSQL) {
    QNetworkRequest request;
    QString strUrl= QString("http://142.93.216.207:8080/selectquery");
    request.setUrl(QUrl(strUrl));

    connect(mpHttpMgr,&QNetworkAccessManager::finished, this, &MainWindow::onQueryResponse);
    mpHttpMgr->put(request, pSQL.toUtf8());
}

void MainWindow::onQueryResponse(QNetworkReply *pReply) {
    disconnect(mpHttpMgr,&QNetworkAccessManager::finished, this, &MainWindow::onQueryResponse);
    ui->tblWdgtReport->setColumnCount(0);
    ui->tblWdgtReport->setRowCount(0);

    QString strResp = pReply->readAll();
    json pRoot      = json::parse(strResp.toStdString(), nullptr, false);
    if(pRoot.is_discarded() || !pRoot.value<bool>("isOk", false)) { updateStatus("No Response"); return; }

    QVector<User::Ptr> users;
    User::Ptr pUser;
    if(!pRoot.contains("users")) { updateStatus("No Users"); return;}
    json pRoots = pRoot["users"];
    if(pRoots.is_array()) for(const auto& root : pRoots) {
        pUser   = User::parseUser(root);
        if(pUser) users.push_back(pUser);
    }

    if(users.size() == 0) updateStatus("No Users");
    else loadTableWidget(users);
}




// ----------------------------------------------------------
//                   Handle UI Changes
// ----------------------------------------------------------

void MainWindow::on_tblWdgtReport_cellClicked(int row, int column) {
    QTableWidgetItem *pItem = ui->tblWdgtReport->item(row, column);
    QString strText         = pItem->text();

    if(strText.length() == 10 && strText.toLongLong() != 0) strText = strText.insert(5, ' ');
    ui->lblStatus->setText(strText);
}

void MainWindow::loadTableWidget(const QVector<User::Ptr>& pUsers) {
    if(pUsers.empty()) return;

    QStringList colHeaders = pUsers[0]->getColumnHeaders();
    ui->tblWdgtReport->setRowCount(pUsers.size());
    ui->tblWdgtReport->setColumnCount(colHeaders.size());

    //  make sure row count and column count are set before setting headers
    ui->tblWdgtReport->setHorizontalHeaderLabels(colHeaders);

    uint32_t iRow = 0;
    for(const auto& pUser : pUsers) {
        uint32_t iCol = 0;
        const auto& pItemWidgets = pUser->getAllFieldsAsWidgetItems();
        for(auto pItemWidget : pItemWidgets) {
            ui->tblWdgtReport->setItem(iRow, iCol++, pItemWidget);
        }
        iRow++;
    }
}

int32_t MainWindow::getMembershipNoFromFileName(QString pFileName) {
    QString strNo;
    if(pFileName.isEmpty()) return 0;
    for(const auto& ch : pFileName) if(ch.isDigit()) strNo += ch;
    if(strNo.isEmpty()) return 0;
    return strNo.toInt();
}

QString MainWindow::getPhoto(int32_t pMembershipNo) {
    if(pMembershipNo <= 0) return QString();
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
    ui->lblStatus->setText(pStatus);
    mpStatusTimer->start(delay);
}

void MainWindow::clearStatus() {
    mpStatusTimer->stop();
    ui->lblStatus->clear();
}

