#ifndef USER_H
#define USER_H

#include <iostream>
#include <QString>
#include "json.hpp"

using namespace nlohmann;
class User
{
    QString     mName, mPhoto, mAddress, mEmail,
                mDOB, mValidityEnd, mLastVisit, mMobile;
    int32_t     mMembershipNo, mId;

public:
    typedef std::shared_ptr<User> Ptr;
    User()
        : mMembershipNo(0)
        , mId(0) {}
    virtual ~User() {}

    json                toJson();
    int32_t             getMembershipNo() { return mMembershipNo; }
    QString             getValidityEnd() { return mValidityEnd; }
    QString             getName() { return mName; }
    static User::Ptr    parseUser(const QString& pUserJson);
};

class Staff {
    QString     mName;
    int32_t     mId, mStaffNo;

public:
    typedef std::shared_ptr<Staff> Ptr;
    Staff() : mId(0), mStaffNo(0) {}
    virtual ~Staff() {}

    static Staff::Ptr   parseStaff(const QString pStaffJson);
    int32_t             getStaffNo() { return mStaffNo; }
    QString             getName() { return mName; }
};

#endif // USER_H
