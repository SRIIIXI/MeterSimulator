#ifndef _CONFIGURATION
#define _CONFIGURATION

#include <string>
#include <filesystem>
#include <map>
#include <vector>
#include <fstream>
#include <algorithm>


class Configuration
{
public:
	Configuration();
	~Configuration();
	void setFileName(std::string fname);
	bool loadConfiguration();
	bool loadCustomConfiguration(const std::string &configFile);
	std::string getValue(const std::string &section, const std::string &settingKey, const std::string defval = "");
	bool isSection(const std::string &section);
private:
	bool loadConfiguration(const std::string &configFile);
	void addSection(std::string &str, const std::map<std::string, std::string> &list);
	std::map<std::string, std::map<std::string, std::string>> _ConfigurationMap;
	std::string _ConfigFileName;
};

#endif
