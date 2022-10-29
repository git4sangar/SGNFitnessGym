//sgn
#ifndef USER_H
#define USER_H

#include <iostream>
#include <QString>
#include <QTableWidgetItem>
#include "nlohmann_json.hpp"

using json = nlohmann::ordered_json;

class User
{
    QString     mName, mPhoto, mAddress, mEmail,
                mDOB, mValidityEnd, mLastVisit, mMobile;
    int32_t     mMembershipNo, mId;

public:
    typedef std::shared_ptr<User> Ptr;
    static constexpr int32_t    SECS_IN_A_DAY       = (24 * 60 * 60);

    User()
        : mMembershipNo(0)
        , mId(0) {}
    virtual ~User() {}

    QStringList                 getColumnHeaders();
    QVector<QTableWidgetItem*>  getAllFieldsAsWidgetItems();

    json                toJson();
    int32_t             getMembershipNo() { return mMembershipNo; }
    QString             getValidityEnd() { return mValidityEnd; }
    QString             getName() { return mName; }
    static User::Ptr    parseUser(const QString& pUserJson);
    static User::Ptr    parseUser(const json& jsRoot);
};

#endif // USER_H
