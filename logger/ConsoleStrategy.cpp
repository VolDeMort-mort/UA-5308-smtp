#include "ConsoleStrategy.h"
#include <iostream>
ConsoleStrategy::ConsoleStrategy()
{
	std::cout<< "Switching to console\n";
}
bool ConsoleStrategy::Write(const std::string& message)
{
	std::cout << message << "\n";
	return true;
}
std::string ConsoleStrategy::SpecificLog(LogLevel lvl, const std::string & msg)
{
	return {};
}
void ConsoleStrategy::Flush()
{
	std::cout.flush();
}
bool ConsoleStrategy::IsValid()
{
	return true;
}