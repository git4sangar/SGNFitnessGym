//sgn

#pragma once

#include <iostream>
#include <vector>
#include <memory>
#include <ctime>

#include "SQLiteCpp/SQLiteCpp.h"
#include "MyDateTime.h"
#include "nlohmann/json.hpp"
#include "Logger.h"

using namespace nlohmann;

struct User {
    typedef std::shared_ptr<User> Ptr;
    User()
    : mMobile(0)
    , mId(0)
    , mMembershipNo(0)
    , mDOB(MyDateTime::INVALID)
    , mValidityEnd(MyDateTime::INVALID)
    , mLastVisit(MyDateTime::INVALID)
    , mLogger(Logger::getInstance()) {}

    std::string         toJson();
    json				toJsonObj();
    std::string         getInsertQueryString();
    std::string			getUpdateQueryString();
    static User::Ptr    parseUser(std::string pUserJson);
    static User::Ptr    parseUser(SQLite::Statement *pQuery);

    std::string mName, mAddress, mEmail, mPhoto;
    uint64_t    mMobile;
    int32_t     mId, mMembershipNo;
    time_t      mDOB, mValidityEnd, mLastVisit;
    Logger& 	mLogger;
};

struct Attendance {
    typedef std::shared_ptr<Attendance> Ptr;
    static constexpr int32_t MIN_WORKOUT_SECs  = (60 * 5);
    Attendance()
    : mId(0)
    , mMembershipNo(0)
    , mInTime(MyDateTime::INVALID)
    , mOutTime(MyDateTime::INVALID)
    , mDuration(0)
    , mLogger(Logger::getInstance()) {}

    std::string toJson();
    static Attendance::Ptr parseAttendance(std::string pAttendanceJson);
    static Attendance::Ptr parseAttendance(SQLite::Statement *pQuery);

    std::string	mInDateString, mOutDateString;
    int32_t 	mId, mMembershipNo, mDuration;
    time_t		mInTime, mOutTime;
    Logger& 	mLogger;
};

struct Fees {
    typedef std::shared_ptr<Fees> Ptr;
    Fees()
    : mId(0)
    , mMembershipNo(0)
    , mSpouseNo(0)
    , mAmountPaid(0)
    , mValidityEnd(MyDateTime::INVALID)
    , mDatePaid(MyDateTime::INVALID)
    , mLogger(Logger::getInstance()) {}

	static Fees::Ptr	parseFees(std::string pFeeJson);
	static Fees::Ptr	parseFees(SQLite::Statement *pQuery);
	std::string			toJson();
	std::string			getInsertQuery();
	std::string			getUpdateQuery();
	
    std::string		mName, mPackage, mReferenceNo, mReceiptNo;
    int32_t			mId, mMembershipNo, mSpouseNo, mAmountPaid;
    time_t			mValidityEnd, mDatePaid;
    Logger& 		mLogger;
};

class DBInterface {
    DBInterface(std::string pDBFileName) : mLogger(Logger::getInstance()) {
        mDB = std::make_shared<SQLite::Database>(pDBFileName, SQLite::OPEN_READWRITE);
    }

    std::shared_ptr<SQLite::Database> mDB;
    Logger& mLogger;
    static DBInterface* pThis;

public:
    typedef std::shared_ptr<DBInterface> Ptr;
    static DBInterface *getInstance( std::string pFileName = "")
        { if(!pThis) { pThis = new DBInterface(pFileName); } return pThis; }
    virtual ~DBInterface() {}

    bool        			addNewUser(User::Ptr pUser);
    bool					updateUser(User::Ptr pUser);
    bool					addNewFees(Fees::Ptr pFee);
    bool					updateFees(Fees::Ptr pFee);
    bool					updateUserValidity(int32_t pMembershipNo, time_t pValidity);
    Fees::Ptr				doesFeeExists(Fees::Ptr pFee);
    User::Ptr   			getUser(int32_t pMembershipNo);
    User::Ptr				getUserByStringField(const std::string& pField, const std::string& pValue);
    int32_t     			newMembershipNo();
    Fees::Ptr				getLastPayDetails(int32_t pMembershipNo);
	std::string				parseQuery(const std::string& pQuery);
    std::vector<User::Ptr>	executeSelectQuery(const std::string& pQuery);
	bool					executeUpdateQuery(const std::string& pQuery);

    bool					markAttendance(int32_t pMembershipNo);
    Attendance::Ptr			getAttendance(uint32_t pMembershipNo);
};
