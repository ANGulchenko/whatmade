#ifndef _IMP_
#define _IMP_

namespace Imp
{
	void stopDaemon();
	void statusDaemon();
	void cleanup();
	bool already_running();
	void daemonize();

	static const char* PID_FILE {"/tmp/whatmade.pid"};
};

#endif
