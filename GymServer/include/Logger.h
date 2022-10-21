//sgn
#pragma once

#include <mutex>
#include <sstream>
#include <iostream>
#include <fstream>

class Logger {

public:
    virtual ~Logger() {}

    Logger& operator<<(const std::string& strLog);
    Logger& operator<<(const size_t val);

    // this is the type of std::cout
    typedef std::basic_ostream<char, std::char_traits<char> > CoutType;
    // this is the function signature of std::endl
    typedef CoutType& (*StandardEndLine)(CoutType&);
    Logger& operator << (StandardEndLine manip);
    void closeLogger() { mLogFile.close(); Logger::pLogger = nullptr; }

    static Logger& getInstance();

private:
    std::ofstream mLogFile;
    std::mutex writeLock;
    bool mbTime;

    Logger();
    void stampTime();
    static Logger* pLogger;
};