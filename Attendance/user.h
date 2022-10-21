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

#endif // USER_H
