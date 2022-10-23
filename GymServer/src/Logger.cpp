//sgn
#include <sstream>
#include <iostream>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>

#include "Logger.h"
#define MODULE_NAME "Logger : "

	Logger* Logger::pLogger = NULL;

	Logger& Logger::getInstance() {
		if (!pLogger) {
			pLogger = new Logger();
			*pLogger << MODULE_NAME << "Logger Started" << std::endl;
		}
		return *pLogger;
	}

	Logger::Logger() : mbTime{ true } {
		time_t t = time(0);
		std::string fileName = "/home/sgn/sgn/projs/SGNFitnessGym/GymServer/logs/log_gym_" + std::to_string(t);
		mLogFile.open(fileName, std::ios::out);
	}

	void Logger::stampTime() {
		struct timeval st;
		gettimeofday(&st, NULL);

		//	why do i need secs since epoch? get secs from now
		//	1662576683 => secs since epoch till now (07-Sep-22 00:05)
		unsigned long secs = st.tv_sec - 1662576683;
		secs = secs % 36000;	// reset secs every 10 hours
		unsigned long msecs = st.tv_usec / 1000;
		unsigned long usecs = st.tv_usec % 1000;
		mLogFile <<  secs << ":" << msecs << ":" << usecs << ": ";
		//std::cout << secs << ":" << msecs << ":" << usecs << ": ";
	}


	Logger& Logger::operator << (StandardEndLine manip) {
		writeLock.lock();
		mLogFile << std::endl; mLogFile.flush();
		//std::cout << std::endl;
		mbTime = true;
		writeLock.unlock();
		return *this;
	}

	Logger& Logger::operator <<(const std::string& strMsg) {
		writeLock.lock();
		if (mbTime) { stampTime(); mbTime = false; }
		mLogFile << strMsg; mLogFile.flush();
		//std::cout << strMsg;
		writeLock.unlock();
		return *this;
	}

	Logger& Logger::operator <<(const size_t iVal) {
		writeLock.lock();
		if (mbTime) { stampTime(); mbTime = false; }
		mLogFile << iVal; mLogFile.flush();
		//std::cout << iVal;
		writeLock.unlock();
		return *this;
	}
