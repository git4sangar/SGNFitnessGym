//sgn
#include <ctime>
#include <memory>
#include <sstream>

#include "nlohmann_json.hpp"

#include "MyDateTime.h"
#include "DBInterface.h"

using json = nlohmann::ordered_json;

DBInterface* DBInterface::pThis = nullptr;

//  Packing shall happen only at the time of construction.
void DBInterface::packReportQueries() {
    mReportQueries  = json::array();

    //  Array of 4 elements each
    json pRoot;

    //  First set of 4 elements
    pRoot = json();
    pRoot["Birthday List"]          = mBDAY_LIST;
    pRoot["Renewals"]               = mRENEWALS;
    pRoot["Attendance Today"]       = mCAME_TODAY;
    pRoot["Attendance on Day"]      = mCAME_ON + "     dd-mm-yyyy";
    mReportQueries.push_back(pRoot);

    //  Second set of 4 elements
    pRoot = json();
    pRoot["Attendance for Month"]   = mCAME_ON + "     mm-yyyy";
    pRoot["Member Attendance"]      = mMEMBER_ATTENDANCE + "  user_no FOR mm-yyyy";
    pRoot["Long Absentees"]         = mLONG_ABSENTEES + "     dd-mm-yyyy";
    //pRoot["List active Members"]	= mACTIVE_MEMBERS;
    pRoot["List all Members"]		= mALL_MEMBERS;
    mReportQueries.push_back(pRoot);

    //  Third set of 4 elements
    pRoot = json();
    pRoot["Monthly Packagers"]		= mMONTHLY_PACKAGE;
    pRoot["Quarterly Packagers"]	= mQUARTERLY_PACKAGE;
    pRoot["Half Yearly Packagers"]	= mHALFYEARLY_PACKAGE;
    pRoot["Annual Packagers"]		= mANNUAL_PACKAGE;
    mReportQueries.push_back(pRoot);

    
    //  Fourth set of 4 elements
    pRoot = json();
    pRoot["New Staff"]		= mNEW_STAFF + "    xxxx";
    pRoot["List Staffs"]	= mLIST_STAFFS;
    pRoot["Staffs Came On"]	= mSTAFFS_ON + "  dd-mm-yyyy";
    pRoot["Staffs On Month"]= mSTAFFS_ON + "  mm-yyyy";
    mReportQueries.push_back(pRoot);
}

User::Ptr DBInterface::getUser(int32_t pMembershipNo) {
    if(pMembershipNo <= 0) return nullptr;

    User::Ptr pUser = nullptr;
    std::stringstream ss;
    ss << "SELECT * FROM User WHERE membership_no = " << pMembershipNo << ";";

    SQLite::Statement query(*mDB, ss.str());
    if(query.executeStep()) {
        pUser   = User::parseUser(&query);
    }
    return pUser;
}

User::Ptr DBInterface::getUserByStringField(const std::string& pField, const std::string& pValue) {
	if(pField.empty() || pValue.empty()) return nullptr;
	
	User::Ptr pUser = nullptr;
    std::stringstream ss;
    ss << "SELECT * FROM User WHERE " << pField << " = \"" << pValue << "\";";

    SQLite::Statement query(*mDB, ss.str());
    if(query.executeStep()) {
        pUser   = User::parseUser(&query);
    }
    return pUser;
}

bool DBInterface::addNewUser(User::Ptr pUser) {
    if(!pUser) return false;

    SQLite::Transaction transaction(*mDB);
    mDB->exec(pUser->getInsertQueryString());
    transaction.commit();
    return true;
}

bool DBInterface::updateUser(User::Ptr pUser) {
    if(!pUser) return false;

    SQLite::Transaction transaction(*mDB);
    mDB->exec(pUser->getUpdateQueryString());
    transaction.commit();
    return true;
}

int32_t DBInterface::newMembershipNo() {
    std::stringstream ss;

    ss << "SELECT MAX(membership_no) FROM user;";
    SQLite::Statement query(*mDB, ss.str());
    if(query.executeStep()) {
        int32_t newMembershipNo = query.getColumn("MAX(membership_no)").getUInt();
        return (newMembershipNo + 1);
    }
    return 0;
}

Attendance::Ptr DBInterface::getAttendance(uint32_t pMembershipNo) {
    if(pMembershipNo == 0) return nullptr;

    std::stringstream ss;
    MyDateTime::Ptr pNow = std::make_shared<MyDateTime>();

    ss.str(""); ss << "SELECT * FROM Attendance WHERE membership_no = " << pMembershipNo
                    << " AND SUBSTR(in_date_string, 1, 10) = \"" << pNow->getDateStr() << "\";";
    SQLite::Statement query(*mDB, ss.str());
    if(query.executeStep()) return Attendance::parseAttendance(&query);
    return nullptr;
}

bool DBInterface::markAttendance(int32_t pMembershipNo) {
    if(pMembershipNo == 0) return false;
    std::stringstream ss;

    MyDateTime::Ptr pNow        = std::make_shared<MyDateTime>();
    Attendance::Ptr pAttendance = getAttendance(pMembershipNo);

    SQLite::Transaction transaction(*mDB);
    if(pAttendance) {
        if(pAttendance->mOutTime != 0) return false; // how come? is he entering again on the same day?
        pAttendance->mOutTime       = pNow->getEpoch();
        pAttendance->mOutDateString = pNow->getDateStr() + std::string(" ") + pNow->getTimeStr();

        pAttendance->mDuration      = pAttendance->mOutTime - pAttendance->mInTime;
        if(pAttendance->mDuration < Attendance::MIN_WORKOUT_SECs)   return true;

        ss.str(""); ss << "UPDATE attendance SET out_time = " << pAttendance->mOutTime
                        << ", out_date_string = \"" << pAttendance->mOutDateString
                        << "\", duration = " << pAttendance->mDuration
                        << " WHERE id = " << pAttendance->mId << ";";
        mDB->exec(ss.str());
    } else {
        pAttendance                 = std::make_shared<Attendance>();
        pAttendance->mMembershipNo  = pMembershipNo;
        pAttendance->mInTime        = pNow->getEpoch();
        pAttendance->mInDateString  = pNow->getDateStr() + std::string(" ") + pNow->getTimeStr();

        ss.str(""); ss << "INSERT INTO attendance (membership_no, in_time, out_time, in_date_string, out_date_string, duration) VALUES ("
                    << pAttendance->mMembershipNo << ", " << pAttendance->mInTime << ", 0, \""
                    << pAttendance->mInDateString << "\", \"-\", "<< pAttendance->mDuration << ");";
        mDB->exec(ss.str());

        ss.str(""); ss << "UPDATE user SET last_visit = " << pAttendance->mInTime
                        << " WHERE membership_no = " << pMembershipNo << ";";
        mDB->exec(ss.str());
    }

    transaction.commit();
    return true;
}

Staff::Ptr DBInterface::getStaff(int32_t pStaffNo) {
    if(pStaffNo <= 0) return nullptr;

    Staff::Ptr pStaff;
    std::stringstream ss;

    ss << "SELECT * FROM staff WHERE staff_no = " << pStaffNo << ";";
    SQLite::Statement query(*mDB, ss.str());
    if(query.executeStep()) pStaff = Staff::parseStaff(&query);

    return pStaff;
}

int32_t DBInterface::markStaffAttendance(int32_t pStaffNo) {
    if(pStaffNo <= 0) return -1;
    std::stringstream ss;

    MyDateTime::Ptr pNow        = std::make_shared<MyDateTime>();
    std::string strDateTimeNow  = pNow->getDateStr() + std::string(" ") + pNow->getTimeStr();
    Staff::Ptr pStaff           = getStaff(pStaffNo);
    StaffAttendance::Ptr pSA    = getStaffAttendance(pStaffNo);
    int32_t duration            = -1;

    SQLite::Transaction transaction(*mDB);
    if(pSA) {
        duration        = pNow->getEpoch() - pSA->mInTime;
        ss.str(""); ss << "UPDATE staffAttendance SET out_time = " << pNow->getEpoch()
                        << ", out_date_string = \"" << strDateTimeNow << "\", duration = " << duration
                        << " WHERE id = " << pSA->mId << ";";
    } else {
        duration        = 0;
        ss.str(""); ss << "INSERT INTO staffAttendance (staff_no, in_time, out_time, in_date_string, name) VALUES ("
                        << pStaffNo << ", " <<  pNow->getEpoch() << ", " << MyDateTime::INVALID
                        << ", \"" << strDateTimeNow << "\", \"" << pStaff->mName << "\" );";
    }
    mDB->exec(ss.str());
    transaction.commit();
    return duration;
}

StaffAttendance::Ptr DBInterface::getStaffAttendance(int32_t pStaffNo) {
    if(pStaffNo <= 0) return nullptr;

    std::stringstream ss;
    MyDateTime::Ptr pNow    = std::make_shared<MyDateTime>();

    ss.str(""); ss << "SELECT * FROM staffAttendance WHERE staff_no = " << pStaffNo
                    << " AND SUBSTR(in_date_string, 1, 10) = \"" << pNow->getDateStr() << "\";";

    SQLite::Statement query(*mDB, ss.str());
    StaffAttendance::Ptr pStaffAttendance, pTempPtr;
    time_t pInTime = MyDateTime::INVALID;

    while(query.executeStep()) {
        pTempPtr = StaffAttendance::parseStaffAttendance(&query);
        if(pTempPtr->mInTime > pInTime) { pStaffAttendance = pTempPtr; pInTime = pTempPtr->mInTime; }
    }

    //  If no record or last record contains out-time, then return null so that a new row is inserted
    if(pStaffAttendance && pStaffAttendance->mOutTime > MyDateTime::INVALID) return nullptr;
    return pStaffAttendance;
}

Fees::Ptr DBInterface::doesFeeExists(Fees::Ptr pFee) {
	if(!pFee) return nullptr;
	
	std::stringstream ss;
	Fees::Ptr pFeeFromDB;
	
	ss.str(""); ss << "SELECT * FROM fees WHERE membership_no = " << pFee->mMembershipNo
					<< " AND " << pFee->mDatePaid << " - date_paid < " << SECS_IN_A_DAY << ";";

	SQLite::Statement query(*mDB, ss.str());
	if(query.executeStep()) pFeeFromDB = Fees::parseFees(&query);
	return pFeeFromDB;
}

bool DBInterface::addNewFees(Fees::Ptr pFee) {
	if(!pFee) return false;

    SQLite::Transaction transaction(*mDB);
    mDB->exec(pFee->getInsertQuery());
    transaction.commit();
    return true;
}

bool DBInterface::updateFees(Fees::Ptr pFee) {
	if(!pFee) return false;

    SQLite::Transaction transaction(*mDB);
    mDB->exec(pFee->getUpdateQuery());
    transaction.commit();
    return true;
}

Fees::Ptr DBInterface::getLastPayDetails(int32_t pMembershipNo) {
	std::stringstream ss;
	Fees::Ptr pFeeFromDB;
	
	ss.str(""); ss << "SELECT * FROM fees WHERE membership_no = " << pMembershipNo
					<< " ORDER BY date_paid DESC;";
	
	SQLite::Statement query(*mDB, ss.str());
	if(query.executeStep()) pFeeFromDB = Fees::parseFees(&query);
	return pFeeFromDB;
}

bool DBInterface::updateUserValidity(int32_t pMembershipNo, time_t pValidity) {
	std::stringstream ss;
	
	SQLite::Transaction transaction(*mDB);
	ss.str(""); ss << "UPDATE user SET validity_end = " << pValidity
					<< " WHERE membership_no = " << pMembershipNo << ";";
	mDB->exec(ss.str());
	transaction.commit();
    return true;
}

json DBInterface::generateAttendanceRport(const std::string& strQuery) {
	if(strQuery.empty()) return json::array();

	json rows = json::array();
	SQLite::Statement query(*mDB, strQuery);
	while(query.executeStep()) {
		Attendance::Ptr pAttendance	= Attendance::parseAttendance(&query);
		if(!pAttendance) continue;

		json row;
		User::Ptr pUser	= getUser(pAttendance->mMembershipNo);
        row["User No"]  = pAttendance->mMembershipNo;
        row["Name"]     = pUser->mName;
        row["Mobile"]   = std::to_string(pUser->mMobile);
        row["Date"]     = pAttendance->mInDateString.length() < MyDateTime::DATE_TIME_LENGTH ? "-" : pAttendance->mInDateString.substr(0,10);
        row["In Time"]  = pAttendance->mInDateString.length() < MyDateTime::DATE_TIME_LENGTH ? "-" : pAttendance->mInDateString.substr(11);
        row["Out Time"] = pAttendance->mOutDateString.length()< MyDateTime::DATE_TIME_LENGTH ? "-" : pAttendance->mOutDateString.substr(11);
        row["Duration"] = MyDateTime::getTimeStr(pAttendance->mDuration);
        row["Expiry"]   = std::make_shared<MyDateTime>(pUser->mValidityEnd)->getDateStr();
        row["Remarks"]  = pUser->mRemarks;
		rows.push_back(row);
	}
	return rows;
}

json DBInterface::generateBDayListReport() {
    std::stringstream ss;

    MyDateTime::Ptr pDateTime	= std::make_shared<MyDateTime>();
    ss.str(""); ss << "SELECT * FROM user WHERE ddMM = \"" << pDateTime->getBDayStr() << "\";";

    json rows = json::array();
    SQLite::Statement query(*mDB, ss.str());
    while(query.executeStep()) {
        User::Ptr pUser	= User::parseUser(&query);
        if(!pUser) continue;

        json row;
        row["User No"]      = pUser->mMembershipNo;
        row["Name"]         = pUser->mName;
        row["Birthday"]     = std::make_shared<MyDateTime>(pUser->mDOB)->getDateStr();
        row["Mobile"]       = std::to_string(pUser->mMobile);
        row["Expiry"]       = std::make_shared<MyDateTime>(pUser->mValidityEnd)->getDateStr();
        row["Remarks"]      = pUser->mRemarks;
        rows.push_back(row);
    }
    return rows;
}

json DBInterface::getUsersForReport(const std::string& pQuery) {
    json rows = json::array();
	if(pQuery.empty()) return rows;

    SQLite::Statement query(*mDB, pQuery);
    while(query.executeStep()) {
        User::Ptr pUser	= User::parseUser(&query);
        if(!pUser) continue;

        json row;
        row["User No"]      = pUser->mMembershipNo;
        row["Name"]         = pUser->mName;
        row["Mobile"]       = std::to_string(pUser->mMobile);
        row["Expiry"]       = std::make_shared<MyDateTime>(pUser->mValidityEnd)->getDateStr();
        row["Last Visit"]   = std::make_shared<MyDateTime>(pUser->mLastVisit)->getDateStr();
        row["In Time"]      = std::make_shared<MyDateTime>(pUser->mLastVisit)->getTimeStr();
        row["Remarks"]      = pUser->mRemarks;
        rows.push_back(row);
    }
    return rows;
}

json DBInterface::getStaffsForReport(const std::string& pQuery) {
    json rows = json::array();
    if(pQuery.empty()) return rows;

    SQLite::Statement query(*mDB, pQuery);
/*
    SELECT staff_no, name, SUBSTR(in_date_string, 1, 10) as date_str, SUM(duration) / 3600 as hours, (SUM(duration) % 3600) / 60 as mins FROM staffAttendance
    WHERE SUBSTR(in_date_string, 1, 10) = "24-10-2022"
    GROUP BY date_str, staff_no
    ORDER BY staff_no ASC;

    SELECT staff_no, name, SUBSTR(in_date_string, 1, 10) as date_str, SUM(duration) / 3600 as hours, (SUM(duration) % 3600) / 60 as mins FROM staffAttendance
    WHERE in_date_string IN (SELECT in_date_string FROM staffAttendance WHERE SUBSTR(in_date_string, 4, 7) = "10-2022")
    GROUP BY date_str, staff_no
    ORDER BY staff_no ASC;
*/

    while(query.executeStep()) {
        json row;
        int hrs, mins;
        row["Staff No"] = query.getColumn("staff_no").getInt();
        row["Name"]     = query.getColumn("name").getString();
        row["Date"]     = query.getColumn("date_str").getString();
        hrs             = query.getColumn("hours").getInt();
        mins            = query.getColumn("mins").getInt();
        row["Duration"] = std::to_string(hrs) + std::string(":") + std::to_string(mins);
        rows.push_back(row);
    }
    return rows;
}

json DBInterface::getRenewalsReport() {
    std::stringstream ss;

    MyDateTime::Ptr pDateTime	= std::make_shared<MyDateTime>();
    ss.str(""); ss  << "SELECT * FROM user WHERE validity_end < "
                    << pDateTime->getEpoch() - SECS_IN_A_DAY
                    << " ORDER BY last_visit DESC;";
    return getUsersForReport(ss.str());
}

json DBInterface::getLongAbsenteesReport(const std::string& pDate) {
    if(pDate.empty()) return json();
    std::stringstream ss;

    //  Get the end of day
    MyDateTime::Ptr pDateTime = MyDateTime::create(pDate, "dd-MM-yyyy", false);
    if(!pDateTime) return json();
    ss.str(""); ss << "SELECT * FROM user WHERE last_visit <= " << pDateTime->getEpoch()
                    << " AND validity_end > " << time(0)
                    << " ORDER BY validity_end DESC;";
    return getUsersForReport(ss.str());
}

json DBInterface::getAllStaffs() {
    json rows = json::array();

    SQLite::Statement query(*mDB, "SELECT * FROM staff;");
    while(query.executeStep()) {
        Staff::Ptr pStaff   = Staff::parseStaff(&query);
        if(!pStaff) continue;

        json row;
        row["Staff No"]     = pStaff->mStaffNo;
        row["Name"]         = pStaff->mName;
        rows.push_back(row);
    }
    return rows;
}

std::string DBInterface::executeUserSelectQuery(const std::string& pQuery) {
    std::string strTemp, strQuery;
    std::stringstream ss;
    time_t tmTemp = 0;
    bool bFlag = false;
    json pRoot;

    strTemp = lowerNoSpace(pQuery);

	MyDateTime::Ptr pDateTime;
    if(strTemp.find(lowerNoSpace(mMEMBERS_CAME)) != std::string::npos) {

        if(strTemp == lowerNoSpace(mCAME_TODAY)) {
            pDateTime	= std::make_shared<MyDateTime>(); bFlag = true;
            ss.str(""); ss << "SELECT * FROM attendance WHERE SUBSTR(in_date_string, 1, 10) = \"" << pDateTime->getDateStr() << "\" ORDER BY in_time DESC;";

        } else if(strTemp == lowerNoSpace(mCAME_YESTERDAY)) {
            pDateTime	= std::make_shared<MyDateTime>(); bFlag = true;
            tmTemp		= pDateTime->getEpoch() - SECS_IN_A_DAY; 
            pDateTime	= std::make_shared<MyDateTime>(tmTemp);
            ss.str(""); ss << "SELECT * FROM attendance WHERE SUBSTR(in_date_string, 1, 10) = \"" << pDateTime->getDateStr() << "\" ORDER BY in_time DESC;";

        } else if(strTemp.find(lowerNoSpace(mCAME_ON)) != std::string::npos) {
            strTemp.erase(0, lowerNoSpace(mCAME_ON).length());
            // Length can be 6 (1-xxxx) 
            if(strTemp.length() == 6) strTemp = std::string("0") + strTemp;

            // Length can be 8 (1-2-xxxx) to 10 (31-12-xxxx)
            if(strTemp.length() > 7 && strTemp.length() < 11 ) {
                pDateTime	= MyDateTime::create(strTemp, "dd-MM-yyyy"); bFlag = true;
                ss.str(""); ss << "SELECT * FROM attendance WHERE SUBSTR(in_date_string, 1, 10) = \"" << pDateTime->getDateStr() << "\" ORDER BY in_time DESC;";

            } else if(strTemp.length() == 7) {
                bFlag = true;
                strTemp[2] = '-';   // 10-2022
                ss.str(""); ss << "SELECT * FROM attendance WHERE SUBSTR(in_date_string, 4, 7) = \"" << strTemp << "\" ORDER BY in_time DESC;";
            }
        } 

        if(bFlag) {
            pRoot["isOk"]	= true;
            pRoot["rows"]	= generateAttendanceRport(ss.str());
            return pRoot.dump();
        }
    } else if(strTemp.find(lowerNoSpace(mMEMBER_ATTENDANCE)) != std::string::npos) {
        //"GET ATTENDANCE FOR MEMBER WITH user_no FOR mm-yyyy"
        strTemp = pQuery;
        strTemp.erase(0, mMEMBER_ATTENDANCE.length());
        auto pos            = strTemp.find("FOR");
        std::string strId   = strTemp.substr(0, pos);
        std::string mmYYYY  = strTemp.substr(pos+4); // ignore "FOR "
        trim(strId); trim(mmYYYY);
        if(mmYYYY.length()  == 6) mmYYYY = std::string("0") + mmYYYY;
        mmYYYY[2] = '-';
        ss.str(""); ss  << "SELECT * FROM attendance WHERE membership_no = " << strId
                        << " AND SUBSTR(in_date_string, 4, 7) = \"" << mmYYYY
                        << "\" ORDER BY in_time DESC;";
        pRoot["isOk"]   = true;
        pRoot["rows"]   = generateAttendanceRport(ss.str());
        return pRoot.dump();
    } else if(strTemp == lowerNoSpace(mBDAY_LIST)) {
        pRoot["isOk"]   = true;
        pRoot["rows"]	= generateBDayListReport();
        return pRoot.dump();
    } else if(strTemp == lowerNoSpace(mRENEWALS)) {
        pRoot["isOk"]   = true;
        pRoot["rows"]   = getRenewalsReport();
        return pRoot.dump();
    } else if(strTemp.find(lowerNoSpace(mLONG_ABSENTEES)) != std::string::npos) {
        strTemp.erase(0, lowerNoSpace(mLONG_ABSENTEES).length());
        // Length can be 8 (1-2-xxxx) to 10 (31-12-xxxx)
        if(strTemp.length() > 7 && strTemp.length() < 11 ) {
            pRoot["isOk"]   = true;
            pRoot["rows"]   = getLongAbsenteesReport(strTemp);
            return pRoot.dump();
        }
    } else if(strTemp == lowerNoSpace(mALL_MEMBERS)) {
        pRoot["isOk"]   = true;
        pRoot["rows"]	= getUsersForReport("SELECT * FROM user ORDER BY last_visit DESC;");
        return pRoot.dump();
    } else if(strTemp == lowerNoSpace(mACTIVE_MEMBERS)) {
        tmTemp			= time(NULL) - (30 * SECS_IN_A_DAY);
        ss.str(""); ss << "SELECT * FROM user WHERE last_visit > " << tmTemp << " ORDER BY last_visit DESC;";
        pRoot["isOk"]   = true;
        pRoot["rows"]   = getUsersForReport(ss.str());
        return pRoot.dump();
    } else if(strTemp.find("packagers") != std::string::npos) {
        pRoot["isOk"]   = true;
        std::string strQuery = "SELECT * FROM user WHERE membership_no IN (SELECT membership_no FROM fees WHERE package LIKE ";
             if(strTemp.find("month") != std::string::npos)  pRoot["rows"] = getUsersForReport(strQuery + "\"%month%\");");
        else if(strTemp.find("quart") != std::string::npos)  pRoot["rows"] = getUsersForReport(strQuery + "\"%quart%\");");
        else if(strTemp.find("half")  != std::string::npos)  pRoot["rows"] = getUsersForReport(strQuery + "\"%half%\");");
        else if(strTemp.find("annual")!= std::string::npos)  pRoot["rows"] = getUsersForReport(strQuery + "\"%annual%\");");
        if(pRoot.contains("rows")) return pRoot.dump();
    } else if(strTemp == lowerNoSpace(mLIST_STAFFS)) {
        pRoot["isOk"]   = true;
        pRoot["rows"]   = getAllStaffs();
        return pRoot.dump();
    } else if(strTemp.find(lowerNoSpace(mNEW_STAFF)) != std::string::npos) {
        json rows   = json::array();
        strTemp     = pQuery;   // Need the name as they typed
        strTemp.erase(0, mNEW_STAFF.length());
        trim(strTemp);  // Remove all spaces in btwn

        //  INSERT INTO staff (staff_no, name) SELECT MAX(staff_no)+1, "StaffName" FROM staff;
        ss.str(""); ss << "INSERT INTO staff (staff_no, name) SELECT MAX(staff_no)+1, \"" << strTemp << "\" FROM staff;";
        executeUpdateQuery(ss.str());

        SQLite::Statement query(*mDB, "SELECT MAX(staff_no) as new_staff_no FROM staff;");
        if(query.executeStep()) {
            json row;
            row["staff_no"] = query.getColumn("new_staff_no").getInt();
            rows.push_back(row);
        }
        pRoot["isOk"]   = true;
        pRoot["rows"]   = rows;
        return pRoot.dump();
    } else if(strTemp.find(lowerNoSpace(mSTAFFS_CAME)) != std::string::npos) {
        strTemp.erase(0, lowerNoSpace(mSTAFFS_ON).length());

        // Length can be 6 (1-xxxx)
        if(strTemp.length() == 6) strTemp = std::string("0") + strTemp;

        // Length can be 8 (1-2-xxxx) to 10 (31-12-xxxx)
        if(strTemp.length() > 7 && strTemp.length() < 11 ) {
            pDateTime   = MyDateTime::create(strTemp, "dd-MM-yyyy");
            ss.str(""); ss  << "SELECT staff_no, name, SUBSTR(in_date_string, 1, 10) as date_str, "
                            << "SUM(duration) / 3600 as hours, (SUM(duration) % 3600) / 60 as mins FROM staffAttendance "
                            << "WHERE SUBSTR(in_date_string, 1, 10) = \"" << pDateTime->getDateStr()
                            << "\" GROUP BY date_str, staff_no ORDER BY staff_no ASC;";
        } else if(strTemp.length() == 7) {
           strTemp[2] = '-';   // 10-2022
           ss.str(""); ss   << "SELECT staff_no, name, SUBSTR(in_date_string, 1, 10) as date_str, "
                            << "SUM(duration) / 3600 as hours, (SUM(duration) % 3600) / 60 as mins FROM staffAttendance "
                            << "WHERE in_date_string IN (SELECT in_date_string FROM staffAttendance WHERE SUBSTR(in_date_string, 4, 7) = \"" << strTemp
                            << "\" ) GROUP BY date_str, staff_no ORDER BY staff_no ASC;";
        }
        mLogger << "Query Staff Attendance " << std::endl << ss.str() << std::endl;
        pRoot["isOk"]   = true;
        pRoot["rows"]   = getStaffsForReport(ss.str());
        return pRoot.dump();
    }

	return std::string();
}

std::vector<User::Ptr> DBInterface::executeSelectQuery(const std::string& pQuery) {
	std::vector<User::Ptr> users;
	std::vector<std::string> colNames;
	User::Ptr pUser;
	
	SQLite::Statement query(*mDB, pQuery);
	while(query.executeStep()) {
		pUser	= User::parseUser(&query);
		users.push_back(pUser);
	}
	return users;
}

bool DBInterface::executeUpdateQuery(const std::string& pQuery) {
    SQLite::Transaction transaction(*mDB);
    int iRowsModified = mDB->exec(pQuery);
    transaction.commit();
    return (iRowsModified > 0);
}




User::Ptr User::parseUser(std::string pUserJson) {
    if(pUserJson.empty()) return nullptr;

    User::Ptr pUser = std::make_shared<User>();
    auto jsRoot = json::parse(pUserJson, nullptr, false);
    if(jsRoot.is_discarded()) return nullptr;
    
    pUser->mId              = jsRoot.value<int32_t>("id", 0);
    pUser->mMembershipNo    = jsRoot.value<int32_t>("membership_no", 0);
    pUser->mName            = jsRoot.value<std::string>("name", "");
    pUser->mAddress         = jsRoot.value<std::string>("address", "");    
    pUser->mEmail           = jsRoot.value<std::string>("email", "");
    pUser->mPhoto           = jsRoot.value<std::string>("photo", "-");

    std::string strMobile, strDOB, strLVisit, strValidity;
    strMobile       = jsRoot.value<std::string>("mobile", "");
    strDOB          = jsRoot.value<std::string>("dob", "");
    strLVisit       = jsRoot.value<std::string>("last_visit", "");
    strValidity     = jsRoot.value<std::string>("validity_end", "");

    if(!strMobile.empty())  pUser->mMobile      = std::stoull(strMobile);
    if(!strDOB.empty())     pUser->mDOB         = MyDateTime::create(strDOB, "dd-MM-yyyy")->getEpoch();
    if(!strLVisit.empty())  pUser->mLastVisit   = MyDateTime::create(strLVisit, "dd-MM-yyyy")->getEpoch();
    if(!strValidity.empty())pUser->mValidityEnd = MyDateTime::create(strValidity, "dd-MM-yyyy")->getEpoch();
    return pUser;
}

User::Ptr User::parseUser(SQLite::Statement *pQuery) {
    if(!pQuery) return nullptr;
    User::Ptr pUser         = std::make_shared<User>();
    pUser->mId              = pQuery->getColumn("id").getInt();
    pUser->mMembershipNo    = pQuery->getColumn("membership_no").getInt();
    
    pUser->mName            = pQuery->getColumn("name").getString();
    pUser->mAddress         = pQuery->getColumn("address").getString();
    pUser->mPhoto           = pQuery->getColumn("photo").getString();
    pUser->mEmail           = pQuery->getColumn("email").getString();
    pUser->mRemarks         = pQuery->getColumn("remarks").getString();

    pUser->mMobile          = pQuery->getColumn("mobile").getInt64();
    pUser->mDOB             = pQuery->getColumn("dob").getInt64();
    pUser->mLastVisit       = pQuery->getColumn("last_visit").getInt64();
    pUser->mValidityEnd     = pQuery->getColumn("validity_end").getInt64();
    return pUser;
}

std::string User::toJson() {
    json jsUser;
    jsUser["id"]            = mId;
    jsUser["membership_no"] = mMembershipNo;

    jsUser["name"]          = mName;
    jsUser["mobile"]        = std::to_string(mMobile);
    jsUser["address"]       = mAddress;
    jsUser["email"]         = mEmail;
    jsUser["photo"]         = mPhoto;
    jsUser["dob"]           = std::make_shared<MyDateTime>(mDOB)->getDateStr();
    jsUser["last_visit"]    = std::make_shared<MyDateTime>(mLastVisit)->getDateStr();
    jsUser["validity_end"]  = std::make_shared<MyDateTime>(mValidityEnd)->getDateStr();

    return jsUser.dump();
}

json User::toJsonObj() {
    json jsUser;
    jsUser["id"]            = mId;
    jsUser["membership_no"] = mMembershipNo;

    jsUser["name"]          = mName;
    jsUser["mobile"]        = std::to_string(mMobile);
    jsUser["address"]       = mAddress;
    jsUser["email"]         = mEmail;
    jsUser["photo"]         = mPhoto;
    jsUser["dob"]           = std::make_shared<MyDateTime>(mDOB)->getDateStr();
    jsUser["last_visit"]    = std::make_shared<MyDateTime>(mLastVisit)->getDateStr();
    jsUser["validity_end"]  = std::make_shared<MyDateTime>(mValidityEnd)->getDateStr();

    return jsUser;
}


std::string User::getInsertQueryString() {
    std::stringstream ss;
    std::string strBDay = std::make_shared<MyDateTime>(mDOB)->getBDayStr();
    ss.str(""); ss << "INSERT INTO user (name, membership_no, dob, ddMM, validity_end, last_visit, address, email, photo, mobile) VALUES ("
                << "\"" << mName << "\", " << mMembershipNo << ", " << mDOB << ", \"" << strBDay << "\", " << mValidityEnd
                << ", " << mLastVisit << ", \"" << mAddress << "\", \"" << mEmail << "\", \"" << mPhoto << "\", \""
                << mMobile << "\");";
    return ss.str();
}

std::string User::getUpdateQueryString() {
    std::stringstream ss;
    ss.str(""); ss << "UPDATE user SET name = \"" << mName
    				<< "\", address = \"" << mAddress << "\", email = \""<< mEmail
    				<< "\", mobile = " << mMobile << ", dob = " << mDOB
    				<< " WHERE id = " << mId << ";";
    return ss.str();
}

Attendance::Ptr Attendance::parseAttendance(std::string pAttendanceJson) {
    if(pAttendanceJson.empty()) return nullptr;
    Attendance::Ptr pAttendance = std::make_shared<Attendance>();

    auto jsRoot = json::parse(pAttendanceJson, nullptr, false);
    if(jsRoot.is_discarded()) return nullptr;

    pAttendance->mId            = jsRoot.value<int32_t>("id", 0);
    pAttendance->mMembershipNo  = jsRoot.value<int32_t>("membership_no", 0);
    pAttendance->mInTime        = jsRoot.value<time_t>("in_time", 0);
    pAttendance->mOutTime       = jsRoot.value<time_t>("out_time", 0);
    pAttendance->mInDateString  = jsRoot.value<std::string>("in_date_string", "");
    pAttendance->mDuration      = jsRoot.value<int32_t>("duration", 0);
    return pAttendance;
}

Attendance::Ptr Attendance::parseAttendance(SQLite::Statement *pQuery) {
    if(!pQuery) return nullptr;
    Attendance::Ptr pAttendance = std::make_shared<Attendance>();
    pAttendance->mId            = pQuery->getColumn("id").getInt();
    pAttendance->mMembershipNo  = pQuery->getColumn("membership_no").getInt();
    pAttendance->mDuration      = pQuery->getColumn("duration").getInt();

    pAttendance->mInTime        = pQuery->getColumn("in_time").getInt64();
    pAttendance->mOutTime       = pQuery->getColumn("out_time").getInt64();

    pAttendance->mInDateString  = pQuery->getColumn("in_date_string").getString();
    pAttendance->mOutDateString = pQuery->getColumn("out_date_string").getString();
    return pAttendance;
}

std::string Attendance::toJson() {
    json jsUser;
    jsUser["id"]                = mId;
    jsUser["membership_no"]     = mMembershipNo;
    jsUser["duration"]          = mDuration;

    jsUser["in_time"]           = mInTime;
    jsUser["out_time"]          = mOutTime;
    jsUser["in_date_string"]    = mInDateString;
    jsUser["out_date_string"]   = mOutDateString;
    return jsUser.dump();
}

Fees::Ptr Fees::parseFees(SQLite::Statement *pQuery) {
	if(!pQuery) return nullptr;
	Fees::Ptr pFee	= std::make_shared<Fees>();
	
	pFee->mId			= pQuery->getColumn("id").getInt();
	pFee->mMembershipNo	= pQuery->getColumn("membership_no").getInt();
	pFee->mSpouseNo		= pQuery->getColumn("spouse_no").getInt();
	pFee->mAmountPaid	= pQuery->getColumn("amount_paid").getInt();
	pFee->mValidityEnd	= pQuery->getColumn("validity_end").getInt64();
	pFee->mDatePaid		= pQuery->getColumn("date_paid").getInt64();
	pFee->mName			= pQuery->getColumn("name").getString();
	pFee->mPackage		= pQuery->getColumn("package").getString();
	pFee->mReferenceNo	= pQuery->getColumn("reference_no").getString();
	pFee->mReceiptNo	= pQuery->getColumn("receipt_no").getString();
	return pFee;
}

Fees::Ptr Fees::parseFees(std::string pFeeJson) {
	if(pFeeJson.empty()) return nullptr;
    Fees::Ptr pFee = std::make_shared<Fees>();
    
    auto jsRoot = json::parse(pFeeJson, nullptr, false);
    if(jsRoot.is_discarded()) return nullptr;
    
    pFee->mId			= jsRoot.value<int32_t>("id", 0);
    pFee->mMembershipNo	= jsRoot.value<int32_t>("membership_no", 0);
    pFee->mSpouseNo		= jsRoot.value<int32_t>("spouse_no", 0);
    pFee->mAmountPaid	= jsRoot.value<int32_t>("amount_paid", 0);
    
    pFee->mName			= jsRoot.value<std::string>("name", "-");
    pFee->mPackage		= jsRoot.value<std::string>("package", "-");
    pFee->mReferenceNo	= jsRoot.value<std::string>("reference_no", "-");
    pFee->mReceiptNo	= jsRoot.value<std::string>("receipt_no", "-");

	std::string strDate;    
	strDate				= jsRoot.value<std::string>("validity_end", "");
    pFee->mValidityEnd	= MyDateTime::create(strDate, "dd-MM-yyyy", false)->getEpoch();
    strDate				= jsRoot.value<std::string>("date_paid", "");
    pFee->mDatePaid		= MyDateTime::create(strDate, "dd-MM-yyyy")->getEpoch();
    
    return pFee;
}

std::string Fees::toJson() {
	json jsRoot;
	jsRoot["id"]			= mId;
	jsRoot["membership_no"]	= mMembershipNo;
	jsRoot["spouse_no"]		= mSpouseNo;
	jsRoot["amount_paid"]	= mAmountPaid;
	jsRoot["name"]			= mName;
	jsRoot["package"]		= mPackage;
	jsRoot["reference_no"]	= mReferenceNo;
	jsRoot["receipt_no"]	= mReceiptNo;
	jsRoot["validity_end"]	= std::make_shared<MyDateTime>(mValidityEnd)->getDateStr();
	jsRoot["date_paid"]		= std::make_shared<MyDateTime>(mDatePaid)->getDateStr();
	
	return jsRoot.dump();
}

std::string Fees::getInsertQuery() {
	std::stringstream ss;
	ss.str(""); ss << "INSERT INTO fees (membership_no, spouse_no, date_paid, amount_paid, validity_end, name, reference_no, receipt_no, package) VALUES ("
					<< mMembershipNo << ", " << mSpouseNo << ", " << mDatePaid << ", " << mAmountPaid << ", " << mValidityEnd << ", \"" << mName << "\", \""<< mReferenceNo
					<< "\", \"" << mReceiptNo << "\", \"" << mPackage << "\");";
	return ss.str();
}

std::string Fees::getUpdateQuery() {
	std::stringstream ss;
	ss.str(""); ss << "UPDATE fees SET date_paid = " << mDatePaid << ", name = \"" << mName
                    << "\", amount_paid = " << mAmountPaid << ", reference_no = \"" << mReferenceNo
                    << "\", receipt_no = \"" << mReceiptNo << "\", package = \"" << mPackage
                    << "\", validity_end = " << mValidityEnd  << ", membership_no = " << mMembershipNo
                    << ", spouse_no = " << mSpouseNo << " WHERE id = " << mId << ";";
	return ss.str();
}

json Staff::toJson() {
    json pRoot;
    pRoot["name"]       = mName;
    pRoot["id"]         = mId;
    pRoot["staff_no"]   = mStaffNo;
    return pRoot;
}

Staff::Ptr Staff::parseStaff(const std::string& pJson) {
    if(pJson.empty()) return nullptr;

    auto jsRoot = json::parse(pJson, nullptr, false);
    if(jsRoot.is_discarded()) return nullptr;

    Staff::Ptr pStaff   = std::make_shared<Staff>();
    pStaff->mName       = jsRoot.value<std::string>("name", "");
    pStaff->mStaffNo    = jsRoot.value<int32_t>("staff_no", 0);
    return pStaff;
}

Staff::Ptr Staff::parseStaff(SQLite::Statement *pQuery) {
    if(!pQuery) return nullptr;


    Staff::Ptr pStaff   = std::make_shared<Staff>();
    pStaff->mId         = pQuery->getColumn("id").getInt();
    pStaff->mName       = pQuery->getColumn("name").getString();
    pStaff->mStaffNo    = pQuery->getColumn("staff_no").getInt();
    return pStaff;
}

json StaffAttendance::toJson() {
    json pRoot;

    pRoot["id"]                 = mId;
    pRoot["staff_no"]           = mStaffNo;

    pRoot["in_time"]            = mInTime;
    pRoot["in_date_string"]     = mInDateString;

    pRoot["out_time"]           = mOutTime;
    pRoot["out_date_string"]    = mOutDateString;
    pRoot["name"]               = mName;

    pRoot["duration"]           = mDuration;
    return pRoot;
}

StaffAttendance::Ptr StaffAttendance::parseStaffAttendance(const std::string& pJson) {
    if(pJson.empty()) return nullptr;

    auto jsRoot = json::parse(pJson, nullptr, false);
    if(jsRoot.is_discarded()) return nullptr;

    StaffAttendance::Ptr pStaffAttendance   = std::make_shared<StaffAttendance>();

    pStaffAttendance->mId                   = jsRoot.value<int32_t>("id", 0);
    pStaffAttendance->mStaffNo              = jsRoot.value<int32_t>("staff_no", 0);

    pStaffAttendance->mInTime               = jsRoot.value<time_t>("in_time", 1);
    pStaffAttendance->mInDateString         = jsRoot.value<std::string>("in_date_string", "");

    pStaffAttendance->mOutTime              = jsRoot.value<time_t>("out_time", 1);
    pStaffAttendance->mOutDateString        = jsRoot.value<std::string>("out_date_string", "");
    pStaffAttendance->mName                 = jsRoot.value<std::string>("name", "");

    pStaffAttendance->mDuration             = jsRoot.value<int32_t>("duration", 0);
    return pStaffAttendance;
}

StaffAttendance::Ptr StaffAttendance::parseStaffAttendance(SQLite::Statement *pQuery) {
    if(!pQuery) return nullptr;

    StaffAttendance::Ptr pStaffAttendance   = std::make_shared<StaffAttendance>();
    pStaffAttendance->mId                   = pQuery->getColumn("id").getInt();
    pStaffAttendance->mStaffNo              = pQuery->getColumn("staff_no").getInt();

    pStaffAttendance->mInTime               = pQuery->getColumn("in_time").getInt64();
    pStaffAttendance->mInDateString         = pQuery->getColumn("in_date_string").getString();

    pStaffAttendance->mOutTime              = pQuery->getColumn("out_time").getInt64();
    pStaffAttendance->mOutDateString        = pQuery->getColumn("out_date_string").getString();
    pStaffAttendance->mName                 = pQuery->getColumn("name").getString();

    pStaffAttendance->mDuration             = pQuery->getColumn("duration").getInt();
    return pStaffAttendance;
}

