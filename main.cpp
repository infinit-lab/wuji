#include <iostream>
#include <fstream>
#include <Windows.h>
#include <string.h>
#include "process_manager.h"
#include "json/json.h"

#define SERVICE_NAME "Wuji"
SERVICE_STATUS serviceStatus = { 0 };
SERVICE_STATUS_HANDLE serviceStatusHandle = nullptr;

void WINAPI serviceMain(int argc, char** argv);
void WINAPI serviceHandler(DWORD fdwControl);

std::ofstream trace;

int main(int argc, const char** argv) {
	char buffer[MAX_PATH + 1] = { 0 };
	GetModuleFileName(NULL, buffer, MAX_PATH);
	char* pt = strrchr(buffer, '\\');
	if (!pt) {
		return -1;
	}
	*pt = '\0';
	SetCurrentDirectory(buffer);

	trace.open("wuji.trace", std::ios::out | std::ios::app);
	if (!trace.is_open()) {
		return -1;
	}

	trace << "Starting..." << std::endl;

	SERVICE_TABLE_ENTRY serviceTable[2] = { 0 };
	serviceTable[0].lpServiceName = (char*)SERVICE_NAME;
	serviceTable[0].lpServiceProc = (LPSERVICE_MAIN_FUNCTION)serviceMain;

	serviceTable[1].lpServiceName = nullptr;
	serviceTable[1].lpServiceProc = nullptr;

	StartServiceCtrlDispatcher(serviceTable);

	trace.close();
	return 0;
}

void WINAPI serviceMain(int argc, char** argv) {
	serviceStatus.dwServiceType = SERVICE_WIN32;
	serviceStatus.dwCurrentState = SERVICE_START_PENDING;
	serviceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
	serviceStatus.dwWin32ExitCode = 0;
	serviceStatus.dwServiceSpecificExitCode = 0;
	serviceStatus.dwCheckPoint = 0;
	serviceStatus.dwWaitHint = 0;
	serviceStatusHandle = RegisterServiceCtrlHandler(SERVICE_NAME, serviceHandler);
	if (serviceStatusHandle == 0) {
		DWORD nError = GetLastError();
		trace << "Failed to RegisterService CtrlHandler. error is " << nError << std::endl;
		return;
	}
	SetServiceStatus(serviceStatusHandle, &serviceStatus);

	char directory[MAX_PATH + 1] = { 0 };
	GetCurrentDirectory(MAX_PATH, directory);
	trace << "Current directory is " << directory << std::endl;

	std::ifstream in;
	in.open("wuji.cfg", std::ios::in);
	if (!in.is_open()) {
		trace << "Failed to open config file." << std::endl;
		serviceStatus.dwCurrentState = SERVICE_STOPPED;
		SetServiceStatus(serviceStatusHandle, &serviceStatus);
		return;
	}
	std::string buffer;
	std::string cache;
	while (in >> buffer) {
		cache.append(buffer);
	}
	in.close();

	trace << "Read Content: " << cache << std::endl;

	Json::CharReaderBuilder builder;
	Json::CharReader* reader = builder.newCharReader();
	Json::Value root;

	int cacheSize = (int)cache.length();
	const char* cacheData = cache.data();

	Json::String errs;
	if (!reader->parse(cacheData, cacheData + cacheSize, &root, &errs)) {
		trace << "Failed to parse config file." << std::endl;
		trace << errs << std::endl;
		serviceStatus.dwCurrentState = SERVICE_STOPPED;
		SetServiceStatus(serviceStatusHandle, &serviceStatus);
		return;
	}
	if (!root.isArray()) {
		trace << "File isn't config file." << std::endl;
		serviceStatus.dwCurrentState = SERVICE_STOPPED;
		SetServiceStatus(serviceStatusHandle, &serviceStatus);
		return;
	}

	std::list<CProcessParam> paramList;
	for (Json::ArrayIndex i = 0; i < root.size(); ++i) {
		CProcessParam param;
		const Json::Value& path = root[i]["path"];
		const Json::Value& dir = root[i]["dir"];
		const Json::Value& params = root[i]["param"];
		if (!path.isString() || !dir.isString()) {
			continue;
		}
		param.path = path.asString();
		param.dir = dir.asString();
		if (params.isString()) {
			param.param = params.asString();
		}
		paramList.push_back(param);
	}

	if (paramList.empty()) {
		trace << "Param list is empty." << std::endl;
		serviceStatus.dwCurrentState = SERVICE_STOPPED;
		SetServiceStatus(serviceStatusHandle, &serviceStatus);
		return;
	}

	CProcessManager::instance()->run(paramList);

	serviceStatus.dwCurrentState = SERVICE_RUNNING;
	serviceStatus.dwCheckPoint = 0;
	serviceStatus.dwWaitHint = 9000;
	if (!SetServiceStatus(serviceStatusHandle, &serviceStatus)) {
		DWORD nError = GetLastError();
		trace << "Failed to SetServiceStatus. error is " << nError << std::endl;
		return;
	}
}

void WINAPI serviceHandler(DWORD fdwControl) {
	switch (fdwControl) {
	case SERVICE_CONTROL_STOP:
	case SERVICE_CONTROL_SHUTDOWN:
		break;
	default:
		return;
	}
	serviceStatus.dwCurrentState = SERVICE_STOP_PENDING;
	SetServiceStatus(serviceStatusHandle, &serviceStatus);

	CProcessManager::instance()->stop();

	serviceStatus.dwWin32ExitCode = 0;
	serviceStatus.dwCheckPoint = 0;
	serviceStatus.dwCurrentState = SERVICE_STOPPED;
	serviceStatus.dwWaitHint = 0;
	if (!SetServiceStatus(serviceStatusHandle, &serviceStatus)) {
		DWORD nError = GetLastError();
		trace << "Failed to SetServiceStatus. error is " << nError << std::endl;
		return;
	}
}
