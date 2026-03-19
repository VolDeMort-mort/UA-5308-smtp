#include "TimeUtils.h"

#include <ctime>
#include <iomanip>
#include <sstream>

namespace SmtpClient {

std::string TimeUtils::get_current_date()
{
	std::time_t now    = std::time(nullptr);
	std::tm     tm_now = {};

#if defined(_WIN32) || defined(_WIN64)
	localtime_s(&tm_now, &now);
#else
	localtime_r(&now, &tm_now);
#endif

	std::ostringstream oss;
	oss << std::put_time(&tm_now, "%a, %d %b %Y %H:%M:%S %z");
	return oss.str();
}

} // namespace SmtpClient
