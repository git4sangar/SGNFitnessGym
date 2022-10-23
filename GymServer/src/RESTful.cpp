//sgn

#include <iostream>
#include <exception>

#include "nlohmann_json.hpp"
#include "RESTful.h"

using json = nlohmann::ordered_json;

void RESTful::configureRoutes()
{
    Pistache::Rest::Routes::Get(mRouter, "/user/:id", Pistache::Rest::Routes::bind(&RESTful::getUser, this));
    Pistache::Rest::Routes::Get(mRouter, "/lastpayment/:id", Pistache::Rest::Routes::bind(&RESTful::getLastPayment, this));
    Pistache::Rest::Routes::Get(mRouter, "/epoch", Pistache::Rest::Routes::bind(&RESTful::getEpochTime, this));
    Pistache::Rest::Routes::Post(mRouter, "/getuser", Pistache::Rest::Routes::bind(&RESTful::getUserByField, this));
    Pistache::Rest::Routes::Get(mRouter, "/newmemberno", Pistache::Rest::Routes::bind(&RESTful::getNewMembershipNo, this));
    Pistache::Rest::Routes::Post(mRouter, "/newuser", Pistache::Rest::Routes::bind(&RESTful::addNewUser, this));
    Pistache::Rest::Routes::Post(mRouter, "/updateuser", Pistache::Rest::Routes::bind(&RESTful::updateUser, this));
    Pistache::Rest::Routes::Post(mRouter, "/addorupdatefee", Pistache::Rest::Routes::bind(&RESTful::addOrUpdateFee, this));

    Pistache::Rest::Routes::Put(mRouter, "/attendance", Pistache::Rest::Routes::bind(&RESTful::putAttendance, this));
    Pistache::Rest::Routes::Put(mRouter, "/selectquery", Pistache::Rest::Routes::bind(&RESTful::executeSelectQuery, this));
}

void RESTful::getEpochTime(const PistacheReq &request, PistacheResp response) {
	mLogger << "Got a request for epoch" << std::endl;
	json pRoot;
	pRoot["epoch"] = time(0);
	response.send(Pistache::Http::Code::Ok, packResponse(true, pRoot.dump()), MIME(Application, Json));
}

void RESTful::getUser(const PistacheReq &request, PistacheResp response) {
	mLogger << "Got user request by membership no" << std::endl;
    try {
        uint32_t membershipNo = request.param(":id").as<uint32_t>();
        User::Ptr pUser = mpDBInterface->getUser(membershipNo);
        if(pUser) {
            response.send(Pistache::Http::Code::Ok, packResponse(true, pUser->toJson()), MIME(Application, Json));
        } else {
            response.send(Pistache::Http::Code::Not_Found, packResponse(false, "Unknown membership no"), MIME(Application, Json));
        }
    } catch(std::exception &e) {
        mLogger << e.what() << std::endl;
        response.send(Pistache::Http::Code::Internal_Server_Error, packResponse(false, "Exception getting user"), MIME(Application, Json));
    }
}

void RESTful::getNewMembershipNo(const PistacheReq &request, PistacheResp response) {
    mLogger << "Got new membership no request" << std::endl;
    int32_t membershipNo        = mpDBInterface->newMembershipNo();
    json pRoot;
    pRoot["new_membership_no"]  = membershipNo;
    response.send(Pistache::Http::Code::Ok, packResponse(true, pRoot.dump()), MIME(Application, Json));
}

void RESTful::addNewUser(const PistacheReq &request, PistacheResp response) {
    mLogger << "Got new user add request" << std::endl;
    User::Ptr pUser, pCheckUser;

    pUser = User::parseUser(request.body());
    if(pUser) {
        pCheckUser = mpDBInterface->getUser(pUser->mMembershipNo);
        if(!pCheckUser) {
            mpDBInterface->addNewUser(pUser);
			pUser = mpDBInterface->getUser(pUser->mMembershipNo);
            response.send(Pistache::Http::Code::Ok, packResponse(true, pUser->toJson()), MIME(Application, Json));
        } else {
            response.send(Pistache::Http::Code::Not_Found, packResponse(false, "Member already existing"), MIME(Application, Json));
        }
    } else {
        response.send(Pistache::Http::Code::Internal_Server_Error, packResponse(false, "Invalid Input"), MIME(Application, Json));
    }
}

void RESTful::updateUser(const PistacheReq &request, PistacheResp response) {
	mLogger << "Got user update request" << std::endl;
	User::Ptr pUser, pCheckUser;

    pUser = User::parseUser(request.body());
    if(pUser) {
        pCheckUser = mpDBInterface->getUser(pUser->mMembershipNo);
        if(pCheckUser) {
            mpDBInterface->updateUser(pUser);
			pUser = mpDBInterface->getUser(pUser->mMembershipNo);
            response.send(Pistache::Http::Code::Ok, packResponse(true, pUser->toJson()), MIME(Application, Json));
        } else {
            response.send(Pistache::Http::Code::Not_Found, packResponse(false, "Member does not exist"), MIME(Application, Json));
        }
    } else {
        response.send(Pistache::Http::Code::Internal_Server_Error, packResponse(false, "Invalid Input"), MIME(Application, Json));
    }
	
}

void RESTful::getUserByField(const PistacheReq &request, PistacheResp response) {
    mLogger << "Got request to get user details by field" << std::endl;
    User::Ptr pUser;
    auto root	= json::parse(request.body(), nullptr, false);
    if(root.is_discarded()) response.send(Pistache::Http::Code::Internal_Server_Error, packResponse(false, "Invalid Input"), MIME(Application, Json));
    
    while(true) {
	    int32_t membershipNo	= root.value<int32_t>("membership_no", 0);
		if(membershipNo > 0)	{ pUser = mpDBInterface->getUser(membershipNo); break; }

		std::string strMobile	= root.value<std::string>("mobile", "");
		if(!strMobile.empty())	{ pUser = mpDBInterface->getUserByStringField("mobile", strMobile); break; }
		
		std::string strEmail	= root.value<std::string>("email", "");
		if(!strEmail.empty())	{ pUser = mpDBInterface->getUserByStringField("email", strEmail); break; }
		
		std::string strName		= root.value<std::string>("name", "");
		if(!strName.empty())	{ pUser = mpDBInterface->getUserByStringField("name", strName); break; }
		
		break;
	}
	if(pUser) response.send(Pistache::Http::Code::Ok, packResponse(true, pUser->toJson()), MIME(Application, Json));
	else response.send(Pistache::Http::Code::Not_Found, packResponse(false, "Unknown member"), MIME(Application, Json));
}

void RESTful::putAttendance(const PistacheReq &request, PistacheResp response) {
    mLogger << "Got PUT request for attendance" << std::endl;
    User::Ptr pUser;
    int32_t membershipNo = 0;
    try {
        auto root   = json::parse(request.body(), nullptr, false);
        if(!root.is_discarded()) {
            membershipNo    = root.value<int32_t>("id", 0);
            pUser           = mpDBInterface->getUser(membershipNo);
        }

        if(pUser) {
            bool isSuccess = mpDBInterface->markAttendance(membershipNo);
            if(!isSuccess) {
                response.send(Pistache::Http::Code::Bad_Request, packResponse(false, "Attendance already marked for today"), MIME(Application, Json));
                return;
            }
            Attendance::Ptr pAttendance = mpDBInterface->getAttendance(membershipNo);
            response.send(Pistache::Http::Code::Ok, packResponse(true, pAttendance->toJson()), MIME(Application, Json));
        } else {
            response.send(Pistache::Http::Code::Not_Found, packResponse(false, "Unknown membership no"), MIME(Application, Json));
        }
    } catch(std::exception &e) {
        mLogger << e.what() << std::endl;
        response.send(Pistache::Http::Code::Internal_Server_Error, packResponse(false, "Exception putting attendance"), MIME(Application, Json));
    }
}

void RESTful::executeSelectQuery(const PistacheReq &request, PistacheResp response) {
	std::string strQuery	= request.body();
		mLogger << "Got a Select Query to execute: " << strQuery << std::endl;

	if(strQuery.empty()) {
		response.send(Pistache::Http::Code::Not_Found, packResponse(false, "Blank Query"), MIME(Application, Json));
		return;
	}
	std::string strQueryResp	= mpDBInterface->executeUserSelectQuery(strQuery);
	if(!strQueryResp.empty()) {
		response.send(Pistache::Http::Code::Ok, strQueryResp, MIME(Application, Json));
	} else {
		std::vector<User::Ptr> users	= mpDBInterface->executeSelectQuery(strQuery);
		if(users.size() == 0)  {
			response.send(Pistache::Http::Code::Not_Found, packResponse(false, "No results"), MIME(Application, Json));
			return;
		}

		json pRoots	= json::array();
		json pRoot;
		for(auto pUser : users) pRoots.push_back(pUser->toJsonObj());
		pRoot["isOk"]	= true;
		pRoot["rows"]	= pRoots;
		response.send(Pistache::Http::Code::Ok, pRoot.dump(), MIME(Application, Json));
	}
}

void RESTful::addOrUpdateFee(const PistacheReq &request, PistacheResp response) {
	mLogger << "Got add or update fee request" << std::endl;
	
	Fees::Ptr pFees, pFeesFromDB;
	pFees		= Fees::parseFees(request.body());
	pFeesFromDB	= mpDBInterface->doesFeeExists(pFees);

	json pRoot;
	if(pFeesFromDB)	{
		pFees->mId		= pFeesFromDB->mId;
		mpDBInterface->updateFees(pFees);
		pRoot["status"]	= "Fee details updated";
	} else {
		mpDBInterface->addNewFees(pFees);
		pRoot["status"]	= "Fee details added";
	}

	mpDBInterface->updateUserValidity(pFees->mMembershipNo, pFees->mValidityEnd);

	if(pFees->mSpouseNo > 0) {
		mpDBInterface->updateUserValidity(pFees->mSpouseNo, pFees->mValidityEnd);
	} else if(pFeesFromDB && pFeesFromDB->mSpouseNo > 0) {
		// means, reverting the wrong spouse entry.
		Fees::Ptr pSpousePrevFee	= mpDBInterface->getLastPayDetails(pFeesFromDB->mSpouseNo);
		if(pSpousePrevFee) { mpDBInterface->updateUserValidity(pFeesFromDB->mSpouseNo, pSpousePrevFee->mValidityEnd); }
		else { mpDBInterface->updateUserValidity(pFeesFromDB->mSpouseNo, (time(0) - SECS_IN_A_DAY)); }
	}

	response.send(Pistache::Http::Code::Ok, packResponse(true, pRoot.dump()), MIME(Application, Json));
}

void RESTful::getLastPayment(const PistacheReq &request, PistacheResp response) {
	mLogger << "Got last payment request" << std::endl;
	
	uint32_t membershipNo	= request.param(":id").as<uint32_t>();
	Fees::Ptr pFees			= mpDBInterface->getLastPayDetails(membershipNo);

	if(pFees)	response.send(Pistache::Http::Code::Ok, packResponse(true, pFees->toJson()), MIME(Application, Json));
	else 		response.send(Pistache::Http::Code::Not_Found, packResponse(false, "Data not found"), MIME(Application, Json));
}

void RESTful::run() {
    //mEndPoint->init(Pistache::Http::Endpoint::options().threads(5));
    mEndPoint->init(Pistache::Http::Endpoint::options().threads(5).flags(Pistache::Tcp::Options::ReuseAddr).flags(Pistache::Tcp::Options::ReusePort));
    configureRoutes();
    mEndPoint->setHandler(mRouter.handler());
    mEndPoint->serve();
}

std::string RESTful::packResponse(bool isSuccess, const std::string& pPkt){
    json pRoot;
    if(isSuccess) {
        pRoot           = json::parse(pPkt);
        pRoot["isOk"]   = isSuccess;
    } else {
        pRoot["isOk"]   = false;
        pRoot["error"]  = pPkt;
    }
    return pRoot.dump();
}
