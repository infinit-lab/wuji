#include <iostream>
#include <Windows.h>
#include "process_manager.h"

#define SERVICE_NAME "Wuji"
SERVICE_STATUS serviceStatus = { 0 };
SERVICE_STATUS_HANDLE serviceStatusHandle = nullptr;

void WINAPI serviceMain(int argc, char** argv);
void WINAPI serviceHandler(DWORD fdwControl);

int main(int argc, const char** argv) {
	SERVICE_TABLE_ENTRY serviceTable[2] = { 0 };
	serviceTable[0].lpServiceName = (char*)SERVICE_NAME;
	serviceTable[0].lpServiceProc = (LPSERVICE_MAIN_FUNCTION)serviceMain;

	serviceTable[1].lpServiceName = nullptr;
	serviceTable[1].lpServiceProc = nullptr;

	StartServiceCtrlDispatcher(serviceTable);
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
		std::cout << "Failed to RegisterService CtrlHandler. error is " << nError << std::endl;
		return;
	}

	CProcessManager::instance()->run(std::list<CProcessParam>());

	serviceStatus.dwCurrentState = SERVICE_RUNNING;
	serviceStatus.dwCheckPoint = 0;
	serviceStatus.dwWaitHint = 9000;
	if (!SetServiceStatus(serviceStatusHandle, &serviceStatus)) {
		DWORD nError = GetLastError();
		std::cout << "Failed to SetServiceStatus. error is " << nError << std::endl;
		return;
	}
}

void WINAPI serviceHandler(DWORD fdwControl) {
	switch (fdwControl) {
	case SERVICE_CONTROL_STOP:
	case SERVICE_CONTROL_SHUTDOWN:
		serviceStatus.dwWin32ExitCode = 0;
		serviceStatus.dwCurrentState = SERVICE_STOP_PENDING;
		serviceStatus.dwCheckPoint = 0;
		serviceStatus.dwWaitHint = 0;
		SetServiceStatus(serviceStatusHandle, &serviceStatus);
		CProcessManager::instance()->stop();
		serviceStatus.dwCurrentState = SERVICE_STOPPED;
		break;
	default:
		return;
	}
	if (!SetServiceStatus(serviceStatusHandle, &serviceStatus)) {
		DWORD nError = GetLastError();
		std::cout << "Failed to SetServiceStatus. error is " << nError << std::endl;
		return;
	}
}
