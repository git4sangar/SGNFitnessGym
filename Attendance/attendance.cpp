//sgn
#include "attendance.h"
#include "json.hpp"

using namespace nlohmann;

Attendance::Ptr Attendance::parseAttendance(const QString& pjsonAttendance) {
    if(pjsonAttendance.isEmpty()) return nullptr;

    Attendance::Ptr pAttendance = std::make_shared<Attendance>();
    auto jsRoot                 = json::parse(pjsonAttendance.toStdString(), nullptr, false);
    if(jsRoot.is_discarded()) return nullptr;

    bool isOk   = jsRoot.value<bool>("isOk", false);
    if(!isOk) return nullptr;

    pAttendance->mId            = jsRoot.value<int32_t>("id", 0);
    pAttendance->mMembershipNo  = jsRoot.value<int32_t>("membership_no", 0);
    pAttendance->mDuration      = jsRoot.value<int32_t>("duration", 0);

    return pAttendance;
}
