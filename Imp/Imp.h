#ifndef _IMP_
#define _IMP_

#include <string>

namespace Imp
{
    void stopDaemon();
	void askDaemonToRereadConfig();
	void statusDaemon();
	void cleanup();
	bool already_running();
	void daemonize();

    static std::string PID_FILE {"/tmp/whomade.pid"};
};

#endif
