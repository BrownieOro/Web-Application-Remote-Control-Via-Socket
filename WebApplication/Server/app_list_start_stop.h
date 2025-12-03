#ifndef APP_LIST_START_STOP_H
#define APP_LIST_START_STOP_H

#include <string>

std::string listApps();
std::string startApp(const std::string& app_name);
std::string stopApp(const std::string& app_name);

#endif
