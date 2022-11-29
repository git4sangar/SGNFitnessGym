//sgn

#pragma once

#include <iostream>
#include <vector>
#include <memory>
#include <ctime>

#include "SQLiteCpp/SQLiteCpp.h"
#include "MyDateTime.h"
#include "nlohmann_json.hpp"
#include "Logger.h"

using json = nlohmann::ordered_json;

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

    std::string mName, mAddress, mEmail, mPhoto, mRemarks;
    uint64_t    mMobile;
    int32_t     mId, mMembershipNo;
    time_t      mDOB, mValidityEnd, mLastVisit;
    Logger& 	mLogger;
};

struct Staff {
    typedef std::shared_ptr<Staff> Ptr;
    Staff()
    : mId(0)
    , mStaffNo(0)
    , mLogger(Logger::getInstance()) {}

    json                toJson();
    static Staff::Ptr   parseStaff(const std::string& pJson);
    static Staff::Ptr   parseStaff(SQLite::Statement *pQuery);

    std::string mName;
    int32_t     mId, mStaffNo;
    Logger&     mLogger;
};

struct StaffAttendance {
    typedef std::shared_ptr<StaffAttendance> Ptr;
    StaffAttendance()
    : mId(0)
    , mStaffNo(0)
    , mInTime(MyDateTime::INVALID)
    , mOutTime(MyDateTime::INVALID)
    , mDuration(0)
    , mLogger(Logger::getInstance()) {}

    json 	toJson();
    static  StaffAttendance::Ptr parseStaffAttendance(const std::string& pJson);
    static  StaffAttendance::Ptr parseStaffAttendance(SQLite::Statement *pQuery);

    std::string mName, mInDateString, mOutDateString;
    int32_t     mId, mStaffNo;
    time_t      mInTime, mOutTime, mDuration;
    Logger&     mLogger;
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
        packReportQueries();
    }

    std::shared_ptr<SQLite::Database> mDB;
    Logger& mLogger;
    json mReportQueries;
    static DBInterface* pThis;

    const std::string mMEMBERS_CAME     = "GET MEMBERS CAME";
    const std::string mCAME_TODAY       = "GET MEMBERS CAME TODAY";
    const std::string mCAME_YESTERDAY   = "GET MEMBERS CAME YESTERDAY";
    const std::string mMEMBER_ATTENDANCE= "GET ATTENDANCE FOR MEMBER WITH";
    const std::string mCAME_ON          = "GET MEMBERS CAME ON";
    const std::string mBDAY_LIST        = "GET BIRTHDAY LIST";
    const std::string mRENEWALS         = "GET RENEWALS";
    const std::string mLONG_ABSENTEES   = "GET MEMBERS WHO DID NOT COME AFTER";
    const std::string mALL_MEMBERS		= "GET ALL MEMBERS";
    const std::string mACTIVE_MEMBERS	= "GET ACTIVE MEMBERS";

	const std::string mMONTHLY_PACKAGE      = "GET MONTHLY PACKAGERS";
	const std::string mQUARTERLY_PACKAGE    = "GET QUARTERLY PACKAGERS";
	const std::string mHALFYEARLY_PACKAGE   = "GET HALFYEARLY PACKAGERS";
	const std::string mANNUAL_PACKAGE       = "GET ANNUAL PACKAGERS";

    const std::string mNEW_STAFF            = "ADD NEW STAFF WITH NAME";
    const std::string mLIST_STAFFS          = "GET ALL STAFFS";
    const std::string mSTAFFS_CAME          = "GET STAFFS CAME";
    const std::string mSTAFFS_ON            = "GET STAFFS CAME ON";

    json generateAttendanceRport(const std::string& strQuery);
    json generateBDayListReport();
    json getRenewalsReport();
    json getLongAbsenteesReport(const std::string& pDate);
    json getUsersForReport(const std::string& pQuery);
    void packReportQueries();

    std::string removeAllSpaces(const std::string& pUserString) {
        std::string strTemp;
        for(const auto& ch : pUserString) if(ch != ' ' && ch != '\t') strTemp += ch;
        return strTemp;
    }
    std::string makeAllLower(const std::string& pString) {
        std::string strTemp = pString;
        std::transform(strTemp.begin(), strTemp.end(), strTemp.begin(), ::tolower);
        return strTemp;
    }
    std::string lowerNoSpace(const std::string& pString) { return removeAllSpaces(makeAllLower(pString)); }
    void trim(std::string &s) {
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {return !std::isspace(ch);}));
        s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {return !std::isspace(ch);}).base(), s.end());
    }
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
	std::string				executeUserSelectQuery(const std::string& pQuery);
    std::vector<User::Ptr>	executeSelectQuery(const std::string& pQuery);
	bool                    executeUpdateQuery(const std::string& pQuery);
	const json&             getReportQueryStrings() { return mReportQueries; }

    bool                    markAttendance(int32_t pMembershipNo);
    Attendance::Ptr         getAttendance(uint32_t pMembershipNo);
    json                    getAllStaffs();
    Staff::Ptr              getStaff(int32_t pStaffNo);
    StaffAttendance::Ptr    getStaffAttendance(int32_t pStaffNo);
    int32_t                 markStaffAttendance(int32_t pStaffNo);
    json                    getStaffsForReport(const std::string& pQuery);
};
