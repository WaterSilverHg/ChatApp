#pragma once
#include"../global.h"
//#include"picojson/picojson.h"

class SettingController {
private:
	picojson::object m_obj{};
public:
	SettingController() = default;
	bool loadFromFile(const char* conf_path);
	double getNum(const char* key);
	std::string getString(const char* key);
};