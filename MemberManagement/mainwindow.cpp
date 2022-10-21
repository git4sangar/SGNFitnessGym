#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QDir>
#include <QFileInfo>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include "json.hpp"

using namespace nlohmann;

const QString MainWindow::BTN_GET_DETAILS   = "Get Details";
const QString MainWindow::BTN_SUBMIT        = "Submit";
const QString MainWindow::BTN_UPDATE        = "Update";

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , mCurrentId(0)
{
    mImagesDir      = QDir::currentPath() + QString("\\..\\Images\\");
    ui->setupUi(this);
    ui->dtEdtDob->setDate(QDate::fromString("15/10/1990", "dd/MM/yyyy"));
    ui->lnEdtMembershipNo->setValidator(new QIntValidator(0, 99999, this));
    ui->lnEdtMobile->setValidator(new QIntValidator(0, 9999999999, this));
    ui->lnEdtLastVisit->setDisabled(true);
    ui->lnEdtRenewal->setDisabled(true);

    setPhoto("logo_01.jpg");
    setWindowIcon(QIcon(mImagesDir + "icon_member.png"));

    mpHttpMgr       = new QNetworkAccessManager();
    mpStatusTimer   = new QTimer();
    connect(mpStatusTimer, SIGNAL(timeout()), this, SLOT(clearStatus()));
}

MainWindow::~MainWindow() {
    delete ui;
}





// ----------------------------------------------------------
//                   Network Requests & Responses
//              142.93.216.207:8080, 192.168.83.12:8080
// ----------------------------------------------------------

void MainWindow::on_btnNewJoiner_clicked() {
    QNetworkRequest request;

    ui->dtEdtDob->setDate(QDate::fromString("15/10/1990", "dd/MM/yyyy"));
    QString strUrl      = QString("http://142.93.216.207:8080/newmemberno");
    request.setUrl(QUrl(strUrl));

    connect(mpHttpMgr, &QNetworkAccessManager::finished, this, &MainWindow::onNewMemberResponse);
    mpHttpMgr->get(request);
}

void MainWindow::onNewMemberResponse(QNetworkReply *pReply) {
    disconnect(mpHttpMgr,&QNetworkAccessManager::finished, this, &MainWindow::onNewMemberResponse);

    int32_t newMemberNo = 0;
    const auto& resp    = QString(pReply->readAll());
    json root           = json::parse(resp.toStdString(), nullptr, false);
    if(!root.is_discarded())
        newMemberNo = root.value<int32_t>("new_membership_no", 0);
    on_btnClear_clicked();
    ui->lnEdtMembershipNo->setText(QString::number(newMemberNo));
    ui->lnEdtMembershipNo->setDisabled(true);

    QDate today = QDate::currentDate();
    ui->lnEdtLastVisit->setText(today.toString("dd-MM-yyyy"));
    ui->lnEdtRenewal->setText(today.toString("dd-MM-yyyy"));

    ui->btnGetDetails->setText(BTN_SUBMIT);
    mCurrentId  = 0;
}

void MainWindow::on_btnGetDetails_clicked() {
    QString strBtnText  = ui->btnGetDetails->text();

         if(strBtnText == BTN_GET_DETAILS)   onGetDetailsClicked();
    else if(strBtnText == BTN_SUBMIT)        onSubmitClicked();
    else if(strBtnText == BTN_UPDATE)        onUpdateClicked();
}

void MainWindow::onGetDetailsClicked() {
    json pRoot;

         if(!ui->lnEdtMembershipNo->text().isEmpty())   pRoot["membership_no"]  = ui->lnEdtMembershipNo->text().toInt();
    else if(!ui->lnEdtMobile->text().isEmpty())         pRoot["mobile"]         = ui->lnEdtMobile->text().toStdString();
    else if(!ui->lnEdtEmail->text().isEmpty())          pRoot["email"]          = ui->lnEdtEmail->text().toStdString();
    else if(!ui->lnEdtName->text().isEmpty())           pRoot["name"]           = ui->lnEdtName->text().toStdString();
    else { updateStatus("Pls enter some fields to search for"); return; }
    QString strJson     = pRoot.dump().c_str();

    QNetworkRequest request;
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QString strUrl  = QString("http://142.93.216.207:8080/getuser");
    request.setUrl(QUrl(strUrl));

    connect(mpHttpMgr, &QNetworkAccessManager::finished, this, &MainWindow::onGetDetailsResponse);
    mpHttpMgr->post(request, strJson.toUtf8());
}

void MainWindow::onGetDetailsResponse(QNetworkReply *pReply) {
    disconnect(mpHttpMgr, &QNetworkAccessManager::finished, this, &MainWindow::onGetDetailsResponse);

    auto strResp    = pReply->readAll();
    auto pRoot      = json::parse(strResp, nullptr, false);
    if(pRoot.is_discarded() || !pRoot.value<bool>("isOk", false)) { updateStatus("Error getting details"); return; }

    mCurrentId          = pRoot.value<int32_t>("id", 0);
    QString strName     = pRoot.value<std::string>("name", "").c_str();
    if(!strName.isEmpty()) ui->lnEdtName->setText(strName);

    int32_t membershipNo= pRoot.value<int32_t>("membership_no", 0);
    if(membershipNo > 0) ui->lnEdtMembershipNo->setText(QString::number(membershipNo));
    QString strPhoto    = getPhoto(membershipNo);
    setPhoto(strPhoto);

    QString strMobile   = pRoot.value<std::string>("mobile", "").c_str();
    if(!strMobile.isEmpty()) ui->lnEdtMobile->setText(strMobile);

    QString strDOB      = pRoot.value<std::string>("dob", "").c_str();
    if(!strDOB.isEmpty()) ui->dtEdtDob->setDate(QDate::fromString(strDOB, "dd-MM-yyyy"));

    QString strEmail    = pRoot.value<std::string>("email", "").c_str();
    if(!strEmail.isEmpty()) ui->lnEdtEmail->setText(strEmail);

    QString strAddress  = pRoot.value<std::string>("address", "").c_str();
    if(!strAddress.isEmpty()) ui->txtEdtAddress->setText(strAddress);

    QString strValidity = pRoot.value<std::string>("validity_end", "").c_str();
    if(!strValidity.isEmpty())ui->lnEdtRenewal->setText(strValidity);

    QString strLastVisit= pRoot.value<std::string>("last_visit", "").c_str();
    if(!strLastVisit.isEmpty())ui->lnEdtLastVisit->setText(strLastVisit);

    ui->lnEdtMembershipNo->setDisabled(true);
    ui->btnGetDetails->setText(BTN_UPDATE);
}

void MainWindow::onSubmitClicked() {
    onSubmitOrUpdate(true);
}

void MainWindow::onUpdateClicked() {
    onSubmitOrUpdate(false);
}

void MainWindow::onSubmitOrUpdate(bool isSubmit) {
    json pRoot;

    if(ui->lnEdtName->text().isEmpty())   { updateStatus("Pls enter name"); return; }
    pRoot["name"]       = ui->lnEdtName->text().toStdString();

    if(ui->lnEdtMobile->text().isEmpty())   { pRoot["mobile"] = ""; }
    else if(ui->lnEdtMobile->text().length() != 10) { updateStatus("Invalid Mobile no"); return; }
    else pRoot["mobile"]= ui->lnEdtMobile->text().toStdString();

    if(ui->lnEdtEmail->text().isEmpty())   { updateStatus("Pls enter email"); return; }
    pRoot["email"]      = ui->lnEdtEmail->text().toStdString();

    if(ui->txtEdtAddress->toPlainText().isEmpty())   { updateStatus("Pls enter address"); return; }
    pRoot["address"]        = ui->txtEdtAddress->toPlainText().toStdString();
    pRoot["validity_end"]   = ui->lnEdtRenewal->text().toStdString();
    pRoot["last_visit"]     = ui->lnEdtLastVisit->text().toStdString();
    pRoot["dob"]            = ui->dtEdtDob->date().toString("dd-MM-yyyy").toStdString();

    if(ui->lnEdtMembershipNo->text().isEmpty()) { updateStatus("Click New Member button to get Membership No"); return; }
    pRoot["membership_no"] = ui->lnEdtMembershipNo->text().toInt();

    pRoot["photo"]      = "-";

    QString strUrl;
    if(isSubmit) {
        strUrl = QString("http://142.93.216.207:8080/newuser");
        connect(mpHttpMgr, &QNetworkAccessManager::finished, this, &MainWindow::onNewUserResponse);
    } else {
        pRoot["id"] = mCurrentId;
        strUrl      = QString("http://142.93.216.207:8080/updateuser");
        connect(mpHttpMgr, &QNetworkAccessManager::finished, this, &MainWindow::onUpdateUserResponse);
    }

    QNetworkRequest request;
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setUrl(QUrl(strUrl));

    QString strJson = pRoot.dump().c_str();
    mpHttpMgr->post(request, strJson.toUtf8());
}

void MainWindow::onNewUserResponse(QNetworkReply *pReply) {
    disconnect(mpHttpMgr, &QNetworkAccessManager::finished, this, &MainWindow::onNewUserResponse);

    auto root = json::parse(pReply->readAll().toStdString(), nullptr, false);
    if(root.is_discarded()) { updateStatus("Error adding user"); return; }

    if(root.value<bool>("isOk", false)) {
        mCurrentId  = root.value<int32_t>("id", 0);
        ui->btnGetDetails->setText(BTN_UPDATE);
        ui->lnEdtMembershipNo->setDisabled(true);
        updateStatus("Successfully added");
        return;
    }
    else updateStatus(root.value<std::string>("error", "Error").c_str());
}

void MainWindow::onUpdateUserResponse(QNetworkReply *pReply) {
    disconnect(mpHttpMgr, &QNetworkAccessManager::finished, this, &MainWindow::onUpdateUserResponse);

    auto root = json::parse(pReply->readAll().toStdString(), nullptr, false);
    if(root.is_discarded()) { updateStatus("Error updating user"); return; }

    if(root.value<bool>("isOk", false)) {
        mCurrentId  = root.value<int32_t>("id", 0);
        ui->lnEdtMembershipNo->setDisabled(true);
        updateStatus("Successfully updated");
        return;
    }
    else updateStatus(root.value<std::string>("error", "Error").c_str());
}






// ----------------------------------------------------------
//                   UI Handling
// ----------------------------------------------------------
void MainWindow::on_btnClear_clicked() {
    mCurrentId  = 0;
    ui->lnEdtMembershipNo->setDisabled(false);
    ui->lnEdtName->clear();
    ui->lnEdtLastVisit->clear();
    ui->lnEdtRenewal->clear();
    ui->lnEdtMembershipNo->clear();
    ui->lnEdtMobile->clear();
    ui->lnEdtEmail->clear();
    ui->txtEdtAddress->clear();
    setPhoto("logo_01.jpg");
    ui->dtEdtDob->setDate(QDate::fromString("15/10/1990", "dd/MM/yyyy"));
    ui->btnGetDetails->setText(BTN_GET_DETAILS);
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
    ui->lblStatus->setText(pStatus);
    mpStatusTimer->start(delay);
}

void MainWindow::clearStatus() {
    mpStatusTimer->stop();
    ui->lblStatus->clear();
}


