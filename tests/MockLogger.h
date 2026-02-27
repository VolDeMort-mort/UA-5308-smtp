#pragma once
#include <iostream>
#include "../logger/ILoggerStrategy.h" 

class MockLogger : public ILoggerStrategy {
public:
    void SpecificLog(LogLevel lvl, const std::string& msg) override {
        std::string levelStr;
        switch (lvl) {
        case PROD:  levelStr = "PROD"; break;
        case DEBUG: levelStr = "DEBUG"; break;
        case TRACE: levelStr = "TRACE"; break;
        default:    levelStr = "NONE"; break;
        }

        Write("[" + levelStr + "] MockLogger: " + msg);
    }

private:
    void Write(const std::string& message) override {
        std::cout << message << "\n";
    }
};