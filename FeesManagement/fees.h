//sgn
#ifndef FEES_H
#define FEES_H

#include <QDate>
#include <QString>
#include <memory>

class Fees
{
public:
    typedef std::shared_ptr<Fees> Ptr;
    QString mName, mPackage, mReferenceNo, mReceiptNo;
    QDate   mValidityEnd, mDatePaid;
    int32_t mId, mMembershipNo, mSpouseNo, mAmountPaid;

    Fees()
    : mId(0)
    , mMembershipNo(0)
    , mSpouseNo(0)
    , mAmountPaid(0) {}

    virtual ~Fees() {}

    static Fees::Ptr    parseFees(QString pJsonFee);
};

#endif // FEES_H
