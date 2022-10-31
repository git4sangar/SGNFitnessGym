#include <QDir>
#include <QDate>
#include <QFileInfo>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QIntValidator>

#include <fees.h>
#include "json.hpp"
#include "mainwindow.h"
#include "./ui_mainwindow.h"

using namespace nlohmann;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow) {
    ui->setupUi(this);

    mImagesDir      = QDir::currentPath() + QString("\\..\\Images\\");
    setPhoto("logo_01.jpg");

    ui->lnEdtMembershipNo->setValidator(new QIntValidator(0, 99999, this));
    ui->lnEdtSpouseNo->setValidator(new QIntValidator(0, 99999, this));
    ui->lnEdtAmtPaid->setValidator(new QIntValidator(0, 99999, this));

    ui->lnEdtName->setEnabled(false);
    ui->lnEdtSpouseName->setEnabled(false);

    ui->dtEdtPayDate->setDate(QDate::currentDate());
    ui->dtEdtPayDate->setEnabled(false);

    mpHttpMgr       = new QNetworkAccessManager();
    mpStatusTimer   = new QTimer();
    connect(mpStatusTimer, SIGNAL(timeout()), this, SLOT(clearStatus()));

    setWindowIcon(QIcon(mImagesDir + "icon_fee.png"));
}

MainWindow::~MainWindow() {
    delete ui;
}

QString MainWindow::getColumnStringValue(QString jsonUser, QString pField, bool showPhoto) {
    auto root = json::parse(jsonUser.toStdString(), nullptr, false);
    if(root.is_discarded()) return QString();

    QString strName;
    if(root.value<bool>("isOk", false)) {
        if(showPhoto) {
            int32_t membershipNo    = root.value<int32_t>("membership_no", 0);
            setPhoto(getPhoto(membershipNo));
        }
        strName = root.value<std::string>(pField.toStdString(), "").c_str();
    }
    return strName;
}



// ----------------------------------------------------------
//                   Network Requests & Responses
//                  142.93.216.207,  192.168.83.12
// ----------------------------------------------------------

void MainWindow::on_lnEdtMembershipNo_returnPressed() {
    setPhoto("logo_01.jpg");
    QNetworkRequest request;

    QString strMemNo= ui->lnEdtMembershipNo->text();
    if(strMemNo.isEmpty()) { updateStatus("Pls Enter Membership no"); return; }

    QString strUrl  = QString("http://142.93.216.207:8080/lastpayment/") + strMemNo;
    request.setUrl(QUrl(strUrl));

    connect(mpHttpMgr,&QNetworkAccessManager::finished, this, &MainWindow::feeDetailsResponse);
    mpHttpMgr->get(request);
}

void MainWindow::feeDetailsResponse(QNetworkReply *pReply) {
    disconnect(mpHttpMgr,&QNetworkAccessManager::finished, this, &MainWindow::feeDetailsResponse);

    Fees::Ptr pFees = Fees::parseFees(pReply->readAll());
    if(pFees) fillUpUI(pFees);
    else getUserFromDB(ui->lnEdtMembershipNo->text(), true);
}

void MainWindow::on_lnEdtSpouseNo_returnPressed() {
    QString membershipNo    = ui->lnEdtSpouseNo->text();
    if(membershipNo.isEmpty()) { updateStatus("Pls Enter Spouse no"); return; }

    getUserFromDB(membershipNo, false);
}

void MainWindow::getUserFromDB(QString pMembershipNo, bool isMember) {
    QNetworkRequest request;
    QString strUrl  = QString("http://142.93.216.207:8080/user/") + pMembershipNo;
    request.setUrl(QUrl(strUrl));
    qDebug() << "Url " << strUrl << ", bool " << isMember;

    (isMember) ?
    connect(mpHttpMgr,&QNetworkAccessManager::finished, this, &MainWindow::onMemberNoResponse):
    connect(mpHttpMgr,&QNetworkAccessManager::finished, this, &MainWindow::onSpouseNoResponse);
    mpHttpMgr->get(request);
}

void MainWindow::onSpouseNoResponse(QNetworkReply *pReply) {
    disconnect(mpHttpMgr,&QNetworkAccessManager::finished, this, &MainWindow::onSpouseNoResponse);

    QString strName = getColumnStringValue(pReply->readAll(), "name");
    if(strName.isEmpty()) { updateStatus("Invalid Membership No"); ui->lnEdtSpouseName->clear(); }
    else ui->lnEdtSpouseName->setText(strName);
}

void MainWindow::onMemberNoResponse(QNetworkReply *pReply) {
    disconnect(mpHttpMgr,&QNetworkAccessManager::finished, this, &MainWindow::onMemberNoResponse);

    QString strResp     = pReply->readAll();
    qDebug() << strResp;
    QString strName     = getColumnStringValue(strResp, "name", true);
    QString strValidity = getColumnStringValue(strResp, "validity_end");
    if(strName.isEmpty() || strValidity.isEmpty()) {
        updateStatus("Invalid Membership No");
        ui->lnEdtName->clear();
    } else {
        ui->lnEdtName->setText(strName);
        ui->dtEdtValidity->setDate(QDate::fromString(strValidity, "dd-MM-yyyy"));
    }
}

void MainWindow::on_btnSubmit_clicked() {
    json pRoot;

    if(ui->lnEdtMembershipNo->text().isEmpty())   { updateStatus("Pls Enter Membership no"); return; }
    pRoot["membership_no"]= ui->lnEdtMembershipNo->text().toInt();

    //  member name and spouse names are disabled. If it has value, it means it's verified
    if(ui->lnEdtSpouseName->text().isEmpty()) pRoot["spouse_no"] = 0;
    else pRoot["spouse_no"]  = ui->lnEdtSpouseNo->text().toInt();

    if(ui->lnEdtName->text().isEmpty())   { updateStatus("Pls Enter Name"); return; }
    pRoot["name"]       = ui->lnEdtName->text().trimmed().toStdString();

    if(ui->lnEdtReceiptNo->text().isEmpty())   { updateStatus("Pls Enter Receipt No"); return; }
    pRoot["receipt_no"] = ui->lnEdtReceiptNo->text().trimmed().toStdString();

    pRoot["date_paid"]  = ui->dtEdtPayDate->date().toString("dd-MM-yyyy").toStdString();

    if(ui->lnEdtAmtPaid->text().isEmpty())   { updateStatus("Pls Enter Amount paid"); return; }
    pRoot["amount_paid"] = ui->lnEdtAmtPaid->text().toInt();

    if(ui->lnEdtRefNo->text().isEmpty())    pRoot["reference_no"] = ui->lnEdtReceiptNo->text().toStdString();
    else pRoot["reference_no"] = ui->lnEdtRefNo->text().trimmed().toStdString();

    QDate validity              = ui->dtEdtValidity->date();
    //if(validity <= QDate::currentDate()) { updateStatus("Validity cannot be less than today's date"); return; }
    //else
    pRoot["validity_end"]  = validity.toString("dd-MM-yyyy").trimmed().toStdString();

    if(ui->lnEdtPackage->text().isEmpty())    { updateStatus("Pls Enter Package Name"); return; }
    else pRoot["package"] = ui->lnEdtPackage->text().trimmed().toStdString();

    QNetworkRequest request;
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setUrl(QUrl("http://142.93.216.207:8080/addorupdatefee"));

    connect(mpHttpMgr,&QNetworkAccessManager::finished, this, &MainWindow::onAddFeeResponse);
    QString strJson = pRoot.dump().c_str();
    mpHttpMgr->post(request, strJson.toUtf8());
}

void MainWindow::onAddFeeResponse(QNetworkReply *pReply) {
    disconnect(mpHttpMgr,&QNetworkAccessManager::finished, this, &MainWindow::onAddFeeResponse);

    QString strResp = pReply->readAll();
    qDebug() << strResp;
    auto root = json::parse(strResp.toStdString(), nullptr, false);
    if(root.is_discarded()) { updateStatus("Error adding fees"); return; }

    if(root.value<bool>("isOk", false)) {
        QString status  = root.value<std::string>("status", "success").c_str();
        updateStatus(status);
        return;
    }
    else updateStatus(root.value<std::string>("error", "Error").c_str());
}





// ----------------------------------------------------------
//                   Handle UI Changes
// ----------------------------------------------------------
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

void MainWindow::on_lnEdtSpouseNo_textChanged(const QString &arg1) {
    if(arg1.isEmpty()) ui->lnEdtSpouseName->clear();
}

void MainWindow::on_lnEdtMembershipNo_textChanged(const QString &arg1) {
    on_btnClear_clicked();
}

void MainWindow::fillUpUI(Fees::Ptr pFees) {
    if(!pFees) return;

    QString strTemp;
    ui->lnEdtName->setText(pFees->mName);

    strTemp = QString::number(pFees->mMembershipNo);
    ui->lnEdtMembershipNo->setText(strTemp);
    setPhoto(getPhoto(pFees->mMembershipNo));

    if(pFees->mSpouseNo > 0) {
        strTemp = QString::number(pFees->mSpouseNo);
        ui->lnEdtSpouseNo->setText(strTemp);
        getUserFromDB(strTemp, false);
    }
    ui->lnEdtReceiptNo->setText(pFees->mReceiptNo);
    ui->dtEdtValidity->setDate(pFees->mDatePaid);

    strTemp   = QString::number(pFees->mAmountPaid);
    ui->lnEdtAmtPaid->setText(strTemp);
    ui->lnEdtRefNo->setText(pFees->mReferenceNo);
    ui->dtEdtValidity->setDate(pFees->mValidityEnd);
    ui->lnEdtPackage->setText(pFees->mPackage);
}

void MainWindow::on_btnClear_clicked() {
    setPhoto("logo_01.jpg");
    //ui->lnEdtMembershipNo->clear();
    ui->lnEdtSpouseNo->clear();
    ui->lnEdtName->clear();
    ui->lnEdtSpouseName->clear();
    ui->lnEdtReceiptNo->clear();
    ui->lnEdtAmtPaid->clear();
    ui->lnEdtRefNo->clear();
    ui->lnEdtPackage->clear();
    ui->dtEdtValidity->setDate(QDate::currentDate());
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

