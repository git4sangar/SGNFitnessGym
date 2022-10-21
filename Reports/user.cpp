//sgn
#include <QStringList>
#include "user.h"
#include "json.hpp"

using namespace nlohmann;

User::Ptr User::parseUser(const QString& pUserJson) {
    if(pUserJson.isEmpty()) return nullptr;

    User::Ptr pUser         = std::make_shared<User>();
    auto jsRoot             = json::parse(pUserJson.toStdString(), nullptr, false);
    if(jsRoot.is_discarded()) return nullptr;

    bool isOk   = jsRoot.value<bool>("isOk", false);
    if(!isOk) return nullptr;

    return parseUser(jsRoot);
}

User::Ptr User::parseUser(const json& jsRoot) {
    if(jsRoot.empty()) return nullptr;

    User::Ptr pUser         = std::make_shared<User>();
    pUser->mId              = jsRoot.value<int32_t>("id", 0);
    pUser->mMembershipNo    = jsRoot.value<int32_t>("membership_no", 0);

    pUser->mName            = QString(jsRoot.value<std::string>("name", "").c_str());
    pUser->mMobile          = QString(jsRoot.value<std::string>("mobile", "").c_str());
    pUser->mPhoto           = QString(jsRoot.value<std::string>("photo", "").c_str());
    pUser->mAddress         = QString(jsRoot.value<std::string>("address", "").c_str());
    pUser->mDOB             = QString(jsRoot.value<std::string>("dob", "").c_str());
    pUser->mEmail           = QString(jsRoot.value<std::string>("email", "").c_str());
    pUser->mValidityEnd     = QString(jsRoot.value<std::string>("validity_end", "").c_str());
    pUser->mLastVisit       = QString(jsRoot.value<std::string>("last_visit", "").c_str());
    return pUser;
}

//  Check the order in which getAllFieldsAsWidgetItems returns
QStringList User::getColumnHeaders() {
    QStringList headerList;

    if(mMembershipNo)           headerList << "User No";
    if(!mName.isEmpty())        headerList << "Name";
    if(!mMobile.isEmpty())      headerList << "Mobile";
    if(!mAddress.isEmpty())     headerList << "Address";
    if(!mDOB.isEmpty())         headerList << "Date Of Birth";
    if(!mEmail.isEmpty())       headerList << "Email";
    if(!mValidityEnd.isEmpty()) headerList << "Expiry";
    if(!mLastVisit.isEmpty())   headerList << "Last Visit";

    return headerList;
}

//  Check the order in which getColumnHeaders returns
QVector<QTableWidgetItem*> User::getAllFieldsAsWidgetItems() {
    QVector<QTableWidgetItem*> allFields;
    QTableWidgetItem* pItem;

    if(mMembershipNo)               { pItem = new QTableWidgetItem(); pItem->setText(QString::number(mMembershipNo)); allFields.push_back(pItem); }
    if(!mName.isEmpty())            { pItem = new QTableWidgetItem(); pItem->setText(mName); allFields.push_back(pItem); }
    if(!mMobile.isEmpty())          { pItem = new QTableWidgetItem(); pItem->setText(mMobile); allFields.push_back(pItem); }
    if(!mAddress.isEmpty())         { pItem = new QTableWidgetItem(); pItem->setText(mAddress); allFields.push_back(pItem); }
    if(!mDOB.isEmpty())             { pItem = new QTableWidgetItem(); pItem->setText(mDOB); allFields.push_back(pItem);}
    if(!mEmail.isEmpty())           { pItem = new QTableWidgetItem(); pItem->setText(mEmail); allFields.push_back(pItem); }
    if(!mValidityEnd.isEmpty())     { pItem = new QTableWidgetItem(); pItem->setText(mValidityEnd); allFields.push_back(pItem); }
    if(!mLastVisit.isEmpty())       { pItem = new QTableWidgetItem(); pItem->setText(mLastVisit); allFields.push_back(pItem); }
    return allFields;
}
