//sgn
#include <QVector>
#include <QDir>
#include <QFileInfo>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>

#include "queryresponseparser.h"
#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "user.h"
#include "nlohmann_json.hpp"

using json = nlohmann::ordered_json;

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
//                  142.93.216.207,  192.168.83.129
// ----------------------------------------------------------

void MainWindow::onEpochResponse(QNetworkReply *pReply) {
    disconnect(mpHttpMgr,&QNetworkAccessManager::finished, this, &MainWindow::onEpochResponse);
    QString strResp = pReply->readAll();

    json pRoot  = json::parse(strResp.toStdString(), nullptr, false);
    if(pRoot.is_discarded()) { mCurEpoch = 0; return; }
    if(pRoot.value<bool>("isOk", false))
        mCurEpoch   = pRoot.value<time_t>("epoch", 0);

    QNetworkRequest request;
    QString strUrl= QString("http://142.93.216.207:8080/getreportqueries");
    request.setUrl(QUrl(strUrl));

    connect(mpHttpMgr,&QNetworkAccessManager::finished, this, &MainWindow::onReportQueries);
    mpHttpMgr->get(request);
}

void MainWindow::onReportQueries(QNetworkReply *pReply) {
    disconnect(mpHttpMgr,&QNetworkAccessManager::finished, this, &MainWindow::onReportQueries);
    QString strResp = pReply->readAll();
    json pRoot      = json::parse(strResp.toStdString(), nullptr, false);
    if(pRoot.is_discarded() || !pRoot.value<bool>("isOk", false)) { updateStatus("No Response"); return; }

    mpReportQueries = std::make_shared<QueryResponseParser>(pRoot);
    if(!mpReportQueries->isOk())    updateStatus("No Query Response");
    else loadQueryWidget();
}

void MainWindow::on_btnQuery_clicked() {
    QString strQuery    = ui->lnEdtQuery->text();
    if(strQuery.isEmpty()) { updateStatus("Pls Enter a Query"); return; }
    if(strQuery.contains("dd-") || strQuery.contains("mm-") || strQuery.contains("yyyy"))
        { updateStatus("Pls enter date/month as suggested"); return; }
    queryDB(strQuery);
    setPhoto("logo_01.jpg");
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
    ui->tblWdgtReport->clear();

    QString strResp = pReply->readAll();
    json pRoot      = json::parse(strResp.toStdString(), nullptr, false);
    if(pRoot.is_discarded() || !pRoot.value<bool>("isOk", false)) { updateStatus("No Response"); return; }

    QueryResponseParser::Ptr pParser    = std::make_shared<QueryResponseParser>(pRoot);
    if(!pParser->isOk())    updateStatus("No Query Response");
    else loadTableWidget(pParser);
}


// ----------------------------------------------------------
//                   Handle UI Changes
// ----------------------------------------------------------

void MainWindow::on_tblWdgtReport_cellClicked(int row, int column) {
    QTableWidgetItem *pItem = ui->tblWdgtReport->item(row, column);
    QString strText         = pItem->text();

    if(strText.length() == 10 && strText.toLongLong() != 0) strText = strText.insert(5, ' ');
    ui->lblStatus->setText(strText);

    //  Set the photograph
    {
        pItem           = ui->tblWdgtReport->item(row, 0);
        strText         = pItem->text();
        QString strPhoto= getPhoto(strText.toInt());
        setPhoto(strPhoto);
    }
}

void MainWindow::loadQueryWidget() {
    if(!mpReportQueries) { updateStatus("No Query Strings"); return; }
    QVector<QTableWidgetItem*> pItems = mpReportQueries->getKeysAsWidgetItems();
    QTableWidgetItem* pItem = nullptr;

    int32_t totalNos    = (int32_t)pItems.size();
    int32_t noOfRows    = mpReportQueries->getNoOfRows();
    int32_t noOfCols    = mpReportQueries->getNoOfCols();

    ui->tblWdgtQuery->setRowCount(noOfRows);
    ui->tblWdgtQuery->setColumnCount(noOfCols);

    for(int32_t iRow        = 0; iRow < noOfRows; iRow++) {
        for(int32_t iCol    = 0; iCol < noOfCols; iCol++) {
            int32_t iPos    = (iRow * noOfCols) + iCol;
            pItem           = (iPos < totalNos) ? pItems[iPos] : nullptr;
            if(pItem) {
                ui->tblWdgtQuery->setItem(iRow, iCol, pItem);
                ui->tblWdgtQuery->setColumnWidth(iCol, 250);
            }
        }
    }
}

void MainWindow::on_tblWdgtQuery_cellClicked(int row, int column) {
    QString strVal  = mpReportQueries->getQueryValue(row, column);
    ui->lnEdtQuery->setText(strVal);
    setPhoto("logo_01.jpg");
}

void MainWindow::loadTableWidget(QueryResponseParser::Ptr pParser) {
    if(!pParser) return;

    QStringList colHeaders          = pParser->getColumnNames();
    QVector<QTableWidgetItem*> pRows= pParser->getFieldsAsWidgetItems();

    int32_t noOfRows    = pRows.size() / colHeaders.size();
    int32_t noOfCols    = colHeaders.size();
    ui->tblWdgtReport->setRowCount(noOfRows);
    ui->tblWdgtReport->setColumnCount(noOfCols);

    //  make sure row count and column count are set before setting headers
    ui->tblWdgtReport->setHorizontalHeaderLabels(colHeaders);

    QTableWidgetItem *pItem = nullptr;
    for(int32_t iRow        = 0; iRow < noOfRows; iRow++) {
        for(int32_t iCol    = 0; iCol < noOfCols; iCol++) {
            int32_t iPos    = (iRow * noOfCols) + iCol;
            pItem           = pRows[iPos];
            ui->tblWdgtReport->setItem(iRow, iCol, pItem);
        }
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
    ui->lblLogo->setPixmap(userImage.scaled(150,150));
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

