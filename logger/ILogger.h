#include <string>

#include "ILoggerStrategy.h"

class ILogger
{
public:
	virtual void Log(LogLevel level, const std::string& message) = 0;
};