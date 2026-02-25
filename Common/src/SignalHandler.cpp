#include "SignalHandler.h"
#include <signal.h>
#include <stdlib.h>

#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

// + BSD specific starts
#ifndef SIGSTKFLT
#define SIGSTKFLT 16
#endif
// + BSD specific ends

int signalNumbers[] = { SIGHUP, SIGINT, SIGQUIT, SIGILL, SIGTRAP, SIGABRT, SIGBUS, SIGFPE, SIGSEGV, SIGPIPE, SIGTERM, SIGSTKFLT, SIGUSR1, SIGUSR2, SIGCHLD };

static void alarmCallback(int sig, siginfo_t *siginfo, void *context);
static void suspendCallback(int sig, siginfo_t *siginfo, void *context);
static void resumeCallback(int sig, siginfo_t *siginfo, void *context);
static void shutdownCallback(int sig, siginfo_t *siginfo, void *context);
static void ignoredCallback(int sig, siginfo_t *siginfo, void *context);
static void resetCallback(int sig, siginfo_t *siginfo, void *context);
static void user1Callback(int sig, siginfo_t *siginfo, void *context);
static void user2Callback(int sig, siginfo_t *siginfo, void *context);
static void childExitCallback(int sig, siginfo_t *siginfo, void *context);
#

SignalCallback *callback = nullptr;

bool isShutdownSignalImpl(const int signum);


SignalHandler::SignalHandler()
{
}

SignalHandler::~SignalHandler()
{
}

void SignalHandler::registerCallbackClient(SignalCallback *clientptr)
{
	callback = clientptr;
}

void SignalHandler::registerSignalHandlers()
{
	struct sigaction act;

	for (int signum = 1; signum < 32; signum++)
	{
		memset(&act, '\0', sizeof(act));
		act.sa_flags = SA_SIGINFO;

		switch (signum)
		{
		case SIGKILL:
		case SIGSTOP:
		{
			continue;
		}
		case SIGINT:
		case SIGQUIT:
		case SIGILL:
		case SIGTRAP:
		case SIGABRT:
		case SIGBUS:
		case SIGFPE:
		case SIGSEGV:
		case SIGPIPE:
		case SIGTERM:
		case SIGSTKFLT:
		{
			act.sa_sigaction = &shutdownCallback;
			break;
		}
		case SIGALRM:
		{
			act.sa_sigaction = &alarmCallback;
			break;
		}
		case SIGTSTP:
		{
			act.sa_sigaction = &suspendCallback;
			break;
		}
		case SIGCONT:
		{
			act.sa_sigaction = &resumeCallback;
			break;
		}
		case SIGHUP:
		{
			act.sa_sigaction = &resetCallback;
			break;
		}
		case SIGCHLD:
		{
			act.sa_sigaction = &childExitCallback;
			break;
		}
		case SIGUSR1:
		{
			act.sa_sigaction = &user1Callback;
			break;
		}
		case SIGUSR2:
		{
			act.sa_sigaction = &user2Callback;
			break;
		}
		default:
		{
			act.sa_sigaction = &ignoredCallback;
		}
		}

		if (sigaction(signum, &act, NULL) < 0)
		{
		}
		else
		{
		}
	}
}

bool SignalHandler::isShutdownSignal(const int signum)
{
	return isShutdownSignalImpl(signum);
}

bool isShutdownSignalImpl(const int signum)
{
	int ctr = 0;

	bool found = false;

	for (ctr = 0; ctr < 6; ctr++)
	{
		if (signalNumbers[ctr] == signum)
		{
			found = true;
			break;
		}
	}

	return found;
}

void suspendCallback(int sig, siginfo_t *siginfo, void *context)
{
	if (callback)
	{
		callback->suspend();
	}
}

void resumeCallback(int sig, siginfo_t *siginfo, void *context)
{
	if (callback)
	{
		callback->resume();
	}
}

void shutdownCallback(int sig, siginfo_t *siginfo, void *context)
{
	if (sig == SIGSEGV)
	{
		exit(1);
	}

	if (callback)
	{
		callback->shutdown();
	}
}

void ignoredCallback(int sig, siginfo_t *siginfo, void *context)
{
	if (isShutdownSignalImpl(sig))
	{
		shutdownCallback(sig, siginfo, context);
		return;
	}
}

void alarmCallback(int sig, siginfo_t *siginfo, void *context)
{
	if (callback)
	{
		callback->alarm();
	}
}

void resetCallback(int sig, siginfo_t *siginfo, void *context)
{
	if (callback)
	{
		callback->reset();
	}
}

void childExitCallback(int sig, siginfo_t *siginfo, void *context)
{
	while (waitpid(-1, NULL, WNOHANG) > 0)
	{
	}

	if (callback)
	{
		callback->childExit();
	}
}

void user1Callback(int sig, siginfo_t *siginfo, void *context)
{
	if (callback)
	{
		callback->userdefined1();
	}
}

void user2Callback(int sig, siginfo_t *siginfo, void *context)
{
	if (callback)
	{
		callback->userdefined2();
	}
}

