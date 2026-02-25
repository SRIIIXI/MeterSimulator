#include "Configuration.h"

#include <iostream>
#include <unistd.h>

Configuration::Configuration()
{
	_ConfigFileName = "";
}

Configuration::~Configuration()
{
}

void Configuration::setFileName(std::string fname)
{
	_ConfigFileName = fname;
}

bool Configuration::isSection(const std::string &section)
{
	std::map<std::string, std::map<std::string, std::string>>::iterator it = _ConfigurationMap.find(section);

	if (it == _ConfigurationMap.end())
	{
		return false;
	}
	else
	{
		return true;
	}
}

std::string Configuration::getValue(const std::string &section, const std::string &settingKey, const std::string defval)
{
	std::string str;

	std::map<std::string, std::map<std::string, std::string>>::iterator it = _ConfigurationMap.find(section);
	if (it == _ConfigurationMap.end())
	{
		return defval;
	}
	else
	{
		std::map<std::string, std::string> subsection = it->second;
		std::map<std::string, std::string>::iterator it2 = subsection.find(settingKey);
		if (it2 == subsection.end())
		{
			return defval;
		}
		str = it2->second;
	}
	return str;
}

bool Configuration::loadCustomConfiguration(const std::string &configFile)
{
	return loadConfiguration(configFile);
}

bool Configuration::loadConfiguration()
{
    std::filesystem::path configPath;

    if (geteuid() == 0)
    {
        // Running as root
        configPath = std::filesystem::path("/etc") / std::string(_ConfigFileName + ".conf");
    }
    else
    {
        // Running as normal user
        const char* home = getenv("HOME");
        if (!home)
        {
            return false;
        }
        configPath = std::filesystem::path(home) / ".config" / std::string(_ConfigFileName + ".conf");
    }

    std::cout << configPath << std::endl;

    return loadConfiguration(configPath.string());
}

bool Configuration::loadConfiguration(const std::string &configFile)
{
    std::ifstream fp(configFile);

    if (!fp.is_open())
        return false;

    std::string line;
    std::vector<std::string> linelist;

    // First pass: load relevant lines
    while (std::getline(fp, line))
    {
        // Trim whitespace
        auto ltrim = line.find_first_not_of(" \t\r\n");
        if (ltrim == std::string::npos)
            continue;
        line = line.substr(ltrim, line.find_last_not_of(" \t\r\n") - ltrim + 1);

        // Skip empty, comment lines
        if (line.empty() || line[0] == ';' || line[0] == '#')
            continue;

        linelist.push_back(line);
    }

    fp.close();

    // Second pass: segregate key-value pairs by section
    std::string curSecHeader;
    std::map<std::string, std::string> kvlist;

    for (const auto &ln : linelist)
    {
        if (ln.front() == '[' && ln.back() == ']')
        {
            // Store previous section before starting a new one
            if (!curSecHeader.empty())
                addSection(curSecHeader, kvlist);

            curSecHeader = ln;
            kvlist.clear();
        }
        else
        {
            // Split key=value
            auto delim = ln.find('=');
            if (delim == std::string::npos)
                continue;

            std::string key   = ln.substr(0, delim);
            std::string value = ln.substr(delim + 1);
            kvlist[key] = value;
        }
    }

    // Store the last section
    if (!curSecHeader.empty())
        addSection(curSecHeader, kvlist);

    return true;
}

void Configuration::addSection(std::string &str, const std::map<std::string, std::string> &list)
{
    str.erase(std::remove_if(str.begin(), str.end(), [](char c){ return c == '[' || c == ']'; }), str.end());
    _ConfigurationMap[str] = list;
}
