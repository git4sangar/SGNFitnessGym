//sgn
#include <QDate>
#include <QDebug>
#include "fees.h"
#include "json.hpp"

using namespace nlohmann;


Fees::Ptr Fees::parseFees(QString pJsonFee){
    if(pJsonFee.isEmpty()) return nullptr;

    qDebug() << pJsonFee;
    json pRoot      = json::parse(pJsonFee.toStdString(), nullptr, false);
    if(pRoot.is_discarded() || !pRoot.value<bool>("isOk", false)) return nullptr;

    Fees::Ptr pFees     = std::make_shared<Fees>();
    pFees->mId          = pRoot.value<int32_t>("id", 0);
    pFees->mMembershipNo= pRoot.value<int32_t>("membership_no", 0);
    pFees->mSpouseNo    = pRoot.value<int32_t>("spouse_no", 0);
    pFees->mAmountPaid  = pRoot.value<int32_t>("amount_paid", 0);

    QString strValidity = pRoot.value<std::string>("validity_end", "").c_str();
    QString strDatePaid = pRoot.value<std::string>("date_paid", "").c_str();

    pFees->mValidityEnd = QDate::fromString(strValidity, "dd-MM-yyyy");
    pFees->mDatePaid    = QDate::fromString(strDatePaid, "dd-MM-yyyy");

    pFees->mName        = QString(pRoot.value<std::string>("name", "").c_str());
    pFees->mPackage     = QString(pRoot.value<std::string>("package", "").c_str());
    pFees->mReferenceNo = QString(pRoot.value<std::string>("reference_no", "").c_str());
    pFees->mReceiptNo   = QString(pRoot.value<std::string>("receipt_no", "").c_str());
    return pFees;
}
