//sgn
#include <iostream>
#include <memory>
#include <fstream>
#include <unistd.h>
#include <sys/types.h>

//	Thread and Socket
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <thread>

#include "RESTful.h"
#include "DBInterface.h"
#include "MyDateTime.h"

#define BIN_FILE_PATH "/home/sgn/sgn/projs/gym/bin/SGNAttendanceBot"
#define MYPORT (60000)

void petWatchDog() {
   int sockfd = 0;
   struct hostent *he;
   sockfd = socket(AF_INET, SOCK_DGRAM, 0);
   struct sockaddr_in their_addr;

   he	= gethostbyname("localhost");
   their_addr.sin_family = AF_INET;      /* host byte order */
   their_addr.sin_port = htons(MYPORT);  /* short, network byte order */
   their_addr.sin_addr = *((struct in_addr *)he->h_addr);
   bzero(&(their_addr.sin_zero), 8);     /* zero the rest of the struct */

	while(true) {
		sendto(sockfd, BIN_FILE_PATH, strlen(BIN_FILE_PATH)+1, 0, (struct sockaddr *)&their_addr, sizeof(struct sockaddr));
		sleep(5);
	}

   close(sockfd);
}

int main() {
    pid_t pid;
    pid = fork();

    //  Exit the parent process
    if(pid > 0) {
        std::cout << "Exiting pid " << getpid() << std::endl;
        exit(EXIT_SUCCESS);
    }

    //  -ve return means error
    if(pid < 0) {
        std::cout << "Error creating child process" << std::endl;
        return -1;
    }

    DBInterface::Ptr pDBInterface = DBInterface::Ptr(DBInterface::getInstance("/home/sgn/sgn/projs/gym/sgndb.db"));

	std::thread watchDogThread(petWatchDog);
	
    RESTful *pRestful = new RESTful(8080, pDBInterface);
    pRestful->run();
}







/*#define		POS_SNO		(0)
#define		POS_NAM		(1)
#define		POS_MEM		(2)
#define		POS_MOB		(3)
#define		POS_DOB		(4)
#define		POS_EXP		(5)
#define		POS_EML		(6)

std::vector<std::string> getTokens(const std::string& strLine) {
    std::vector<std::string> tokens;
    std::string strToken;

	char bk;
    for (auto ch : strLine) {
    	bk = ch;
    	if(ch == ' ' || ch == '\t') continue;
        if (ch == ',') {
            tokens.push_back(strToken);
            strToken.clear();
        }
        else strToken += ch;
    }
    if (!strToken.empty() || bk == ',') tokens.push_back(strToken);
    return tokens;
}

int main() {
	DBInterface::Ptr pDBInterface = DBInterface::Ptr(DBInterface::getInstance("/home/tstone10/sgn/proj/gym/sgndb.db"));
	std::ifstream iTestStream("members.csv");
	
	std::string strLine;
	std::vector<std::string> tokens;
	std::stringstream ss;
	
	uint32_t iCount = 0;
	while (std::getline(iTestStream, strLine)) {
		if(strLine.empty()) continue;
		tokens	= getTokens(strLine);
		
		MyDateTime::Ptr pDateTime;
		pDateTime			= MyDateTime::create(tokens[POS_DOB], "dd-MM-yyyy");
		time_t dob			= (pDateTime) ? pDateTime->getEpoch() : (time(0) - (25 * SECS_IN_YEAR));
		std::string strBDay = std::make_shared<MyDateTime>(dob)->getBDayStr();
		
		pDateTime			= MyDateTime::create(tokens[POS_EXP], "dd-MM-yyyy");
		time_t validity_end	= (pDateTime) ? pDateTime->getEpoch() : (time(0) - SECS_IN_A_DAY);
		
		time_t last_visit	= time(0)- SECS_IN_A_DAY;
		std::string strEmail= tokens[POS_EML].empty() ? "-" : tokens[POS_EML];
		uint64_t mobile		= tokens[POS_MOB].empty() ? 1000010000 : std::stoll(tokens[POS_MOB]);

		ss.str(""); ss << "INSERT INTO user (name, membership_no, dob, ddMM, validity_end, last_visit, address, email, photo, mobile) VALUES ("
                		<< "\"" << tokens[POS_NAM] << "\", " << tokens[POS_MEM] << ", " << dob << ", \"" << strBDay << "\", " << validity_end
                		<< ", " << last_visit << ", \"-\", \"" << strEmail << "\", \"-\", \"" << mobile << "\");";
		pDBInterface->executeUpdateQuery(ss.str());
		iCount++;
	}
	std::cout << "Inserted " << iCount << " rows" << std::endl;
	return 0;
}*/


