//sgn
#ifndef ATTENDANCE_H
#define ATTENDANCE_H

#include <QString>
#include <memory>
#include <ctime>

class Attendance
{
    int32_t mId, mMembershipNo, mDuration;

public:
    typedef std::shared_ptr<Attendance> Ptr;
    Attendance()
        : mId(0)
        , mDuration(0)
    {}
    virtual ~Attendance() {}

    int32_t     getDuration() { return mDuration; }
    static Ptr  parseAttendance(const QString& pjsonAttendance);
};


#endif // ATTENDANCE_H
