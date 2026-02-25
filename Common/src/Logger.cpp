#include "Logger.h"
#include <stdarg.h>
#include <stdio.h>

#include <unistd.h>
#include <string.h>

Logger objLogger;

Logger*  Logger::GetInstance()
{
	return &objLogger;
}

Logger::Logger()
{
	_LogDirectory = "";
	_LogFileSize = 1024;

	char pidstr[16];
	memset((char*)&pidstr[0], 0, 16);
	sprintf(pidstr, "%d", getpid());
	_ModuleName = pidstr;

	_LogLevelMap.clear();

	LogLevel l1 = LogInfo;
	LogLevel l2 = LogWarning;
	LogLevel l3 = LogError;
	LogLevel l4 = LogCritical;

	_LogLevelMap[l1] = "Information";
	_LogLevelMap[l2] = "Warning    ";
	_LogLevelMap[l3] = "Error      ";
	_LogLevelMap[l4] = "Critical   ";
}

Logger::~Logger()
{
	stopLogging();
}

void Logger::stopLogging()
{
	if (_LogFile.is_open())
	{
		_LogFile.close();
	}
	_LogLevelMap.clear();
}

void Logger::createBackupFileName(std::string &str)
{
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
    localtime_r(&time, &tm);  // use localtime_s(&tm, &time) on MSVC

    std::ostringstream oss;
    oss << _ModuleName << '_'
        << std::put_time(&tm, "%Y.%m.%d-%H.%M.%S")
        << ".log";

    str = oss.str();
}

void Logger::startLogging(LogFileMode fmode)
{
	_FileMode = fmode;

	if (_LogDirectory.length() < 1)
	{
		char filepathbuffer[1024];
		memset((char*)&filepathbuffer[0], 0, 1024);
		getcwd(&filepathbuffer[0], 1024);

		std::string pdir = std::filesystem::path(filepathbuffer).parent_path().string();

		pdir += "config/";

		if (!std::filesystem::exists(pdir))
		{
			std::filesystem::create_directories(pdir);
		}

		_LogDirectory = filepathbuffer;
	}

	_LogFilename = _LogDirectory + _ModuleName + ".log";

	if (_FileMode == FileAppend)
	{
		_LogFile.open(_LogFilename, std::ios::out | std::ios::app);
	}
	else
	{
		_LogFile.open(_LogFilename, std::ios::out | std::ios::trunc);
	}
}

void Logger::write(std::string logEntry, LogLevel llevel, const char* func, const char* file, int line)
{
	if (_LogFile.is_open())
	{
		int sz = _LogFile.tellp();

		if (sz >= _LogFileSize * 1024)
		{
			std::string temp;
			createBackupFileName(temp);
			std::string backupfile = _LogBackupDirectory + temp;
			stopLogging();
			rename(_LogFilename.c_str(), backupfile.c_str());
			startLogging(_FileMode);
		}

		std::string sourcefile = std::filesystem::path(file).filename().string();
		std::string lvel = _LogLevelMap[llevel];

		auto now  = std::chrono::system_clock::now();
		auto time = std::chrono::system_clock::to_time_t(now);
		std::tm tm{};
		localtime_r(&time, &tm);

		std::ostringstream oss;
		oss << std::put_time(&tm, "%Y.%m.%d-%H.%M.%S");
		std::string tstamp = oss.str();

		char temp[1024];
		memset((char*)&temp[0], 0, 16);

		std::string fname = func;

		// Strip parameters: "Foo::Bar(char* abc)" -> "Foo::Bar"
		auto paren = fname.find('(');
		if (paren != std::string::npos)
		{
			fname = fname.substr(0, paren);
		}

		// Strip namespace: "Foo::Bar" -> "Bar"
		auto colons = fname.rfind("::");
		if (colons != std::string::npos)
		{
			fname = fname.substr(colons + 2);
		}

		// Strip return type if present: "void Bar" -> "Bar"
		auto space = fname.rfind(' ');
		if (space != std::string::npos)
		{
			fname = fname.substr(space + 1);
		}

		sprintf(temp, "%s|%s|%05d|%s|%s| ", tstamp.c_str(), lvel.c_str(), line, fname.c_str(), sourcefile.c_str());

		std::string tbuff(temp);
		_LogFile.write(tbuff.c_str(), tbuff.length());
		_LogFile.write(logEntry.c_str(), logEntry.length());
	}
}

void Logger::setModuleName(std::string mname)
{
    _ModuleName = std::filesystem::path(mname).filename().string();
}

void Logger::setLogFileSize(int flsz)
{
	_LogFileSize = flsz;
}

void Logger::setLogDirectory(std::string dirpath)
{
	_LogDirectory = dirpath;

	int len = _LogDirectory.length();

	char buffer[2048] = { 0 };

	memcpy(buffer, _LogDirectory.c_str(), len);

	if (buffer[len - 1] == '/' || buffer[len - 1] == '\\')
	{
		buffer[len - 1] = 0;
	}

	memcpy(buffer + len, ".bak/", 5);

	_LogBackupDirectory = buffer;

	if (!std::filesystem::exists(buffer))
	{
		std::filesystem::create_directories(buffer);
	}
}

void Logger::writeExtended(LogLevel llevel, const char *func, const char *file, int line, const char* format, ...)
{
	char tempbuf[1024];
	memset((char*)&tempbuf[0], 0, 1024);
	va_list args;
	va_start(args, format);
	vsprintf(tempbuf, format, args);
	tempbuf[1023] = 0;
	write(tempbuf, llevel, func, file, line);
}

