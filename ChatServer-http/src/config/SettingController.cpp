#include"SettingController.h"

//constexpr const char* CONF_PATH = "../conf.json";

bool SettingController::loadFromFile(const char * conf_path) {
	std::ifstream ifs(conf_path, std::ios::in);
	if (!ifs.is_open()) {
		return false;
	}
	//std::string jsonStr;
	std::string jsonStr((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
	//std::cout << jsonStr;

	picojson::value v;
	std::string err;
	picojson::parse(v, jsonStr.begin(), jsonStr.end(), &err);

	if (!err.empty()) {
		std::cerr << "Error parsing JSON: " << err << std::endl;
		return false;
	}

	if (!v.is<picojson::object>()) {
		std::cerr << "JSON is not an object" << std::endl;
		return false;
	}
		
	m_obj = v.get<picojson::object>();

	return true;
}

double SettingController::getNum(const char* key) {
	if (m_obj.count(key) && m_obj[key].is<double>()) {
		return m_obj[key].get<double>();
	}
	else {
		throw std::runtime_error("Key not found or not an integer");
	}
}


std::string SettingController::getString(const char* key) {
	if (m_obj.count(key) && m_obj[key].is<std::string>()) {
		return m_obj[key].get<std::string>();
	}
	else {
		throw std::runtime_error("Key not found or not a string");
	}
}