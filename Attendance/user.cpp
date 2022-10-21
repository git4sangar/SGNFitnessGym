//sgn
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
