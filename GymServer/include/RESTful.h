//sgn
#pragma once

#include <memory>

#include <pistache/endpoint.h>
#include <pistache/http.h>
#include <pistache/mime.h>
#include <pistache/net.h>
#include <pistache/router.h>

#include "nlohmann_json.hpp"
#include "DBInterface.h"
#include "Logger.h"

using json = nlohmann::ordered_json;

class RESTful {
public:
    using PistacheReq   = Pistache::Rest::Request;
    using PistacheResp  = Pistache::Http::ResponseWriter;

    RESTful(uint32_t pPort, DBInterface::Ptr pDBInterface)
    : mpDBInterface(pDBInterface)
//    , mEndPoint(std::make_shared<Pistache::Http::Endpoint>(Pistache::Address("192.168.83.129", pPort)))
    , mEndPoint(std::make_shared<Pistache::Http::Endpoint>(Pistache::Address("142.93.216.207", pPort)))
    , mLogger(Logger::getInstance())
    {}

    virtual ~RESTful() {}
    void run();

private:
    void configureRoutes();
    void getUser(const PistacheReq &request, PistacheResp response);
    void addNewUser(const PistacheReq &request, PistacheResp response);
    void updateUser(const PistacheReq &request, PistacheResp response);
    void putAttendance(const PistacheReq &request, PistacheResp response);
    void getNewMembershipNo(const PistacheReq &request, PistacheResp response);
    void getUserByField(const PistacheReq &request, PistacheResp response);
    void addOrUpdateFee(const PistacheReq &request, PistacheResp response);
    void getLastPayment(const PistacheReq &request, PistacheResp response);
    void getEpochTime(const PistacheReq &request, PistacheResp response);
    void executeSelectQuery(const PistacheReq &request, PistacheResp response);
    void getReportQueries(const PistacheReq &request, PistacheResp response);
    std::string packResponse(bool isSuccess, const std::string& pPkt);
    //std::string packResponse(bool isSuccess, nlohmann::json& pPkt);

    DBInterface::Ptr		mpDBInterface;
    Pistache::Rest::Router	mRouter;
    std::shared_ptr<Pistache::Http::Endpoint> mEndPoint;
    Logger& 				mLogger;
};
