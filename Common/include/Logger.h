#ifndef _LOGGER
#define _LOGGER

#include <string>
#include <map>
#include <filesystem>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <ostream>
#include <fstream>

#define FUNCTIONNAME __PRETTY_FUNCTION__

typedef enum LogLevel
{
	LogInfo = 0,
	LogError = 1,
	LogWarning = 2,
	LogCritical = 3
}LogLevel;

typedef enum LogFileMode
{
	FileAppend = 0,
	FileCreateNew = 1
}LogFileMode;

typedef std::map<LogLevel, std::string> LogLevelStrings;

class Logger
{
public:
	Logger();
	~Logger();

	void   startLogging(LogFileMode fmode);
	void   stopLogging();
	void   write(std::string logEntry, LogLevel llevel, const char* func, const char* file, int line);
	void   writeExtended(LogLevel llevel, const char* func, const char* file, int line, const char* format, ...);
	void   setLogFileSize(int flsz);
	void   setLogDirectory(std::string dirpath);
	void   setModuleName(std::string mname);
	static Logger*  GetInstance();
private:
	void createBackupFileName(std::string &str);
	int				_RemoteLogPort;
	std::string		_LogFilename;
	std::string		_RemoteLogHost;
	std::string		_LogDirectory;
	std::string		_LogBackupDirectory;
	int				_LogFileSize;
	std::string		_ModuleName;
	std::fstream	_LogFile;
	LogFileMode		_FileMode;
	LogLevelStrings	_LogLevelMap;
};

#define writeLog(str,level) Logger::GetInstance()->write(str,level,FUNCTIONNAME,__FILE__,__LINE__);
#define writeLogNormal(str) Logger::GetInstance()->write(str,LogInfo,FUNCTIONNAME,__FILE__,__LINE__);

#endif

