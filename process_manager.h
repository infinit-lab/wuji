#pragma once
#include <string>
#include <list>

struct CProcessParam {
	std::string path;
	std::string param;
	std::string dir;
};

struct CProcess {
	CProcessParam param;
	void* process;
	void* thread;
	bool isStarted;
};

class CProcessManager {
private:
	CProcessManager();
	~CProcessManager();

public:
	static CProcessManager* instance();

	bool run(const std::list<CProcessParam>& paramList);
	void stop();
	void createProcess(CProcess* process);
	void terminateProcess(CProcess* process);

private:
	static CProcessManager* m_instance;
	bool m_isRunning;
	std::list<CProcess*> m_processList;
};
