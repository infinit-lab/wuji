#include "process_manager.h"
#include <Windows.h>
#include <thread>
#include <fstream>

extern std::ofstream trace;

CProcessManager* CProcessManager::m_instance = nullptr;

CProcessManager::CProcessManager()
	: m_isRunning(false)
	, m_processList() {

}

CProcessManager::~CProcessManager() {

}

CProcessManager* CProcessManager::instance() {
	if (!m_instance) {
		m_instance = new CProcessManager();
	}
	return m_instance;
}

bool CProcessManager::run(const std::list<CProcessParam>& paramList) {
	if (m_isRunning) {
		return true;
	}
	m_isRunning = true;

	for (auto it = paramList.begin(); it != paramList.end(); it++) {
		CProcess* process = new CProcess;
		process->param = *it;
		process->process = INVALID_HANDLE_VALUE;
		process->thread = INVALID_HANDLE_VALUE;
		process->isStarted = false;
		m_processList.push_back(process);
	}

	for (auto it = m_processList.begin(); it != m_processList.end(); it++) {
		createProcess(*it);
	}
	return true;
}

void CProcessManager::stop() {
	if (!m_isRunning) {
		return;
	}
	m_isRunning = false;
	for (auto it = m_processList.begin(); it != m_processList.end(); it++) {
		terminateProcess(*it);
	}
	for (int i = 0; i < 300; ++i) {
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		for (auto it = m_processList.begin(); it != m_processList.end(); it++) {
			if ((*it)->isStarted) {
				continue;
			}
		}
		break;
	}

	for (auto it = m_processList.begin(); it != m_processList.end(); it++) {
		delete *it;
	}
	m_processList.clear();
}

void CProcessManager::createProcess(CProcess* process) {
	std::thread([=]() {
		process->isStarted = true;
		while (m_isRunning) {
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			STARTUPINFO si = { 0 };
			PROCESS_INFORMATION pi = { 0 };
			si.cb = sizeof(STARTUPINFO);
			trace << "path is " << process->param.path;
			trace << " param is " << process->param.param;
			trace << " dir is " << process->param.dir << std::endl;
			BOOL ret = CreateProcess(process->param.path.data(), (char*)process->param.param.data()
				, NULL, NULL, FALSE, CREATE_SUSPENDED | CREATE_NO_WINDOW, NULL, process->param.dir.data(), &si, &pi);
			if (ret) {
				trace << "Success to CreateProcess. " << process->param.path << std::endl;
				process->process = pi.hProcess;
				process->thread = pi.hThread;
				ResumeThread(process->thread);
				WaitForSingleObject(process->process, INFINITE);
				CloseHandle(process->thread);
				CloseHandle(process->process);
				process->thread = INVALID_HANDLE_VALUE;
				process->process = INVALID_HANDLE_VALUE;
			}
			else {
				DWORD error = GetLastError();
				trace << "Failed to CreateProcess " << process->param.path << ". error: " << error << std::endl;
			}
		}
		trace << process->param.path << " Quit." << std::endl;
		process->isStarted = false;
	}).detach();
}

void CProcessManager::terminateProcess(CProcess* process) {
	if (process->process == INVALID_HANDLE_VALUE) {
		return;
	}
	DWORD exitCode = 0;
	BOOL ret = GetExitCodeProcess(process->process, &exitCode);
	if (!ret) {
		return;
	}
	if (exitCode == STILL_ACTIVE) {
		TerminateProcess(process->process, -1);
	}
}


