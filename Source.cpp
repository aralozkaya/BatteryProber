/*
    BatteryProber - Execute Programs on AC Power Changes
    Copyright (C) 2022  Ibrahim Aral Ozkaya

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "resource.h"
#include "myButton.h"
#include "pugixml.hpp"
#include <Windows.h>
#include <shobjidl.h> 
#include <vector>
#include <Lmcons.h>
#pragma warning(disable : 4996)

LRESULT CALLBACK mainWindowProc(HWND, UINT, WPARAM, LPARAM);
void showAboutMessageBox(HWND hwnd);
bool checkExistingTask();
bool elevatePrompt(HWND hwnd);

static int argc;
static LPWSTR* argv;

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd) {
	LPCWSTR lpwCmdLine = GetCommandLine();

	argv = CommandLineToArgvW(lpwCmdLine, &argc);

	SYSTEM_POWER_STATUS systemPowerStatus = {};
	bool res = GetSystemPowerStatus(&systemPowerStatus);

	if (!res) {
		MessageBox(NULL, TEXT("Error Getting System Power Status"), TEXT("ERROR"), MB_ICONERROR);
	}
	
	switch (argc) {
		case 1: 
		{
			WNDCLASS MainWindow = {};

			MainWindow.hInstance = hInstance;
			MainWindow.lpfnWndProc = mainWindowProc;
			MainWindow.lpszClassName = TEXT("MainWindow");
			MainWindow.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
			MainWindow.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));

			RegisterClass(&MainWindow);

			HWND hMainWindow = CreateWindow(
				TEXT("MainWindow"),
				TEXT("Battery Prober"),
				WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
				CW_USEDEFAULT, CW_USEDEFAULT, 400, 250,
				NULL,
				NULL,
				hInstance,
				NULL
			);

			if (hMainWindow == NULL) {
				MessageBox(NULL, TEXT("Error Creating Window"), TEXT("ERROR"), MB_ICONERROR);
				return EXIT_FAILURE;
			}

			if (ShowWindow(hMainWindow, nShowCmd)) {
				MessageBox(NULL, TEXT("Error Showing Window"), TEXT("ERROR"), MB_ICONERROR);
				return EXIT_FAILURE;
			}

			MSG msg = { };
			while (GetMessage(&msg, NULL, 0, 0) > 0)
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}

			break;
		}

		case 3:
		{
			WCHAR path[MAX_PATH + 2] = TEXT("");

			wcscat(path, TEXT("\""));

			if (systemPowerStatus.ACLineStatus == '\x1') {
				wcscat(path, argv[1]);
			}
			else {
				wcscat(path, argv[2]);
			}

			wcscat(path, TEXT("\""));

			ShellExecute(NULL, TEXT("open"), path, NULL, NULL, nShowCmd);

			break;
		}

		default: {
			showAboutMessageBox(NULL);
		}
	}

	LocalFree(argv);
	return EXIT_SUCCESS;
}

LRESULT CALLBACK mainWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
		case WM_CREATE: {
			RECT winRect = {};
			GetClientRect(hwnd, &winRect);

			WCHAR ScheduleButtonText[50] = TEXT("");

			if (checkExistingTask()) {
				wcscpy(ScheduleButtonText, TEXT("Delete Prober Schedule"));
			}
			else {
				wcscpy(ScheduleButtonText, TEXT("Schedule Prober to Run"));
			}
			
			//Setting height || width to -1 sets them automatically using text font dimensions
			myButton* ScheduleButton = new myButton(ScheduleButtonText, -1, 300, hwnd, (HMENU)BTN_SCHEDULE);
			myButton* checkACButton= new myButton(TEXT("Check AC Status"), -1, 300, hwnd, (HMENU) BTN_CHECKSTATUS);
			myButton* aboutButton = new myButton(TEXT("About the Usage"), -1, 300, hwnd, (HMENU) BTN_ABOUT);
			
			int totalHeight = 0;
			for each (myButton* button in myButton::getButtonList())
			{
				totalHeight += button->getHeight() + 10;
			}
			totalHeight -= 10;

			int startOfButtonBlock_y = ((winRect.bottom - winRect.top) - totalHeight) / 2;
			for (int i = 0; i < myButton::getButtonList().size(); i++) {
				myButton* button = myButton::getButtonList()[i];
				if (i == 0) {
					button->create(((winRect.right - winRect.left) - button->getWidth()) / 2, startOfButtonBlock_y);
					continue;
				}
				myButton* prevButton = myButton::getButtonList()[i - 1];
				button->create(((winRect.right - winRect.left) - button->getWidth()) / 2, prevButton->bottomPos() + 10);
			}

			return 0;
		}

		case WM_COMMAND: {
			if (LOWORD(wParam) == BTN_SCHEDULE && HIWORD(wParam) == BN_CLICKED) {
				WCHAR proberPath[MAX_PATH] = TEXT("");
				GetEnvironmentVariable(TEXT("WINDIR"), proberPath, MAX_PATH);
				wcscat(proberPath, TEXT("\\System32\\prober.exe"));
				
				if (checkExistingTask()) {
					if (!DeleteFile(proberPath)) {
						if (GetLastError() == 5) {
							if (elevatePrompt(hwnd))	ExitProcess(5);
							else return 0;
						}
					}

					int choice = MessageBox(hwnd, TEXT("Are You Sure You Want To Delete the Task?"), TEXT("Warning"), MB_YESNO | MB_ICONWARNING);
					if (choice != IDYES) return 0;

					SHELLEXECUTEINFO pExecInfo = {};
					pExecInfo.cbSize = sizeof(pExecInfo);
					pExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
					pExecInfo.lpVerb = TEXT("open");
					pExecInfo.lpFile = TEXT("schtasks.exe");
					pExecInfo.lpParameters = TEXT("/delete /tn \"BatteryProber\" /f");
					pExecInfo.lpDirectory = NULL;
					pExecInfo.nShow = SW_HIDE;

					ShellExecuteEx(&pExecInfo);
					if (pExecInfo.hProcess) {
						WaitForSingleObject(pExecInfo.hProcess, INFINITE);
						if (!checkExistingTask()) {
							MessageBox(hwnd, TEXT("The Prober Task Has Been Successfully Deleted"), TEXT("Notice"), MB_ICONINFORMATION);
							myButton* schButton = myButton::findButtonByHMENU((HMENU)BTN_SCHEDULE);
							schButton->changeText(TEXT("Schedule Prober to Run"));
						}
						else {
							MessageBox(hwnd, TEXT("Unknown Error While Deleting Task"), TEXT("ERROR"), MB_ICONERROR);
							return EXIT_FAILURE;
						}
					}
					else {
						MessageBox(hwnd, TEXT("Unknown Error While Deleting Task"), TEXT("ERROR"), MB_ICONERROR);
						return EXIT_FAILURE;
					}

				}
				else {
					if (!CopyFile(argv[0], proberPath, FALSE)) {
						if (GetLastError() == 5) {
							if (elevatePrompt(hwnd))	ExitProcess(5);
							else return 0;
						}
					}

					WCHAR probeArgumentsW[2 * MAX_PATH] = TEXT("");

					HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
					IFileDialog* openArgDialog = NULL;
					hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_IFileOpenDialog, (LPVOID*)&openArgDialog);
					COMDLG_FILTERSPEC acceptedTypes[] = {
						{TEXT("Batch Files"), TEXT("*.bat")},
						{TEXT("Executables"), TEXT("*.exe")}
					};
					openArgDialog->SetFileTypes(2, acceptedTypes);
					openArgDialog->SetTitle(TEXT("Choose Program to Launch when AC Connects"));
					openArgDialog->Show(hwnd);
					IShellItem* pItem;
					if (SUCCEEDED(openArgDialog->GetResult(&pItem))) {
						PWSTR dialogFilePath;
						if (SUCCEEDED(pItem->GetDisplayName(SIGDN_FILESYSPATH, &dialogFilePath))) {
							wcscat(probeArgumentsW, TEXT("\""));
							wcscat(probeArgumentsW, dialogFilePath);
							wcscat(probeArgumentsW, TEXT("\""));
							CoTaskMemFree(dialogFilePath);
							openArgDialog->SetTitle(TEXT("Choose Program to Launch when AC Disconnects"));
							openArgDialog->Show(hwnd);
							if (SUCCEEDED(openArgDialog->GetResult(&pItem))) {
								if (SUCCEEDED(pItem->GetDisplayName(SIGDN_FILESYSPATH, &dialogFilePath))) {
									wcscat(probeArgumentsW, TEXT(" \""));
									wcscat(probeArgumentsW, dialogFilePath);
									wcscat(probeArgumentsW, TEXT("\""));
									CoTaskMemFree(dialogFilePath);
								}
								else {
									MessageBox(hwnd, TEXT("Cannot Get Selected File's Path"), TEXT("ERROR"), MB_ICONERROR);
									CoUninitialize();
									return EXIT_FAILURE;
								}
							}
							else {
								CoUninitialize();
								return 0;
							}
						}
						else {
							MessageBox(hwnd, TEXT("Cannot Get Selected File's Path"), TEXT("ERROR"), MB_ICONERROR);
							CoUninitialize();
							return EXIT_FAILURE;
						}
					}
					else {
						CoUninitialize();
						return 0;
					}

					CoUninitialize();

					WCHAR userName[UNLEN + 1];
					DWORD userNameMaxLen = UNLEN + 1;
					GetUserName(userName, &userNameMaxLen);

					WCHAR computerName[UNLEN + 1];
					DWORD computerNameMaxLen = UNLEN + 1;
					GetComputerName(computerName, &computerNameMaxLen);

					WCHAR authorNameforTaskW[2 * (UNLEN + 1)] = TEXT("");
					wcscat(authorNameforTaskW, computerName);
					wcscat(authorNameforTaskW, TEXT("\\"));
					wcscat(authorNameforTaskW, userName);

					char authorNameforTaskA[2 * (UNLEN + 1)];
					wcstombs(authorNameforTaskA, authorNameforTaskW, size_t(2 * (UNLEN + 1)));

					char probeArgumentsA[2 * MAX_PATH];
					wcstombs(probeArgumentsA, probeArgumentsW, size_t(2 * MAX_PATH));

					HRSRC hsrcTaskXML = FindResource(NULL, MAKEINTRESOURCE(XML_TASKTEMPLATE), TEXT("XML"));

					if (hsrcTaskXML) {
						DWORD dwTaskXML = SizeofResource(NULL, hsrcTaskXML);
						HGLOBAL hTaskXML = LoadResource(NULL, hsrcTaskXML);
						if (hTaskXML) {
							LPVOID lpTaskXML = LockResource(hTaskXML);
							HANDLE fhTaskXML = CreateFile(TEXT("BatteryProbeTask.xml"), GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
							WriteFile(fhTaskXML, lpTaskXML, dwTaskXML, NULL, NULL);
							CloseHandle(fhTaskXML);
							FreeResource(hTaskXML);
						}
						else {
							MessageBox(hwnd, TEXT("Error Loading Template Task XML"), TEXT("ERROR"), MB_ICONERROR);
							return EXIT_FAILURE;
						}
					}
					else {
						MessageBox(hwnd, TEXT("Error Finding Template Task XML Resource"), TEXT("ERROR"), MB_ICONERROR);
						return EXIT_FAILURE;
					}

					pugi::xml_document doc;
					doc.load_file("BatteryProbeTask.xml", pugi::parse_default | pugi::parse_declaration, pugi::encoding_utf16_le);
					pugi::xml_node author = doc.child("Task").child("RegistrationInfo").child("Author").first_child();
					author.set_value(authorNameforTaskA);
					pugi::xml_node arguments = doc.child("Task").child("Actions").child("Exec").child("Arguments").first_child();
					arguments.set_value(probeArgumentsA);
					doc.save_file("BatteryProbeTask.xml", "	", pugi::format_default | pugi::format_write_bom, pugi::encoding_utf16_le);

					SHELLEXECUTEINFO pExecInfo = {};
					pExecInfo.cbSize = sizeof(pExecInfo);
					pExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
					pExecInfo.lpVerb = TEXT("open");
					pExecInfo.lpFile = TEXT("schtasks.exe");
					pExecInfo.lpParameters = TEXT("/create /tn \"BatteryProber\" /xml \"BatteryProbeTask.xml\"");
					pExecInfo.lpDirectory = NULL;
					pExecInfo.nShow = SW_HIDE;
					
					ShellExecuteEx(&pExecInfo);
					if (pExecInfo.hProcess) {
						WaitForSingleObject(pExecInfo.hProcess, INFINITE);
						DeleteFile(TEXT("BatteryProbeTask.xml"));
						if (checkExistingTask()) {
							MessageBox(hwnd, TEXT("The Prober Task Has Been Successfully Created"), TEXT("Notice"), MB_ICONINFORMATION);
							myButton* schButton = myButton::findButtonByHMENU((HMENU)BTN_SCHEDULE);
							schButton->changeText(TEXT("Delete Prober Schedule"));
						}
						else {
							MessageBox(hwnd, TEXT("Unknown Error While Generating Task"), TEXT("ERROR"), MB_ICONERROR);
							return EXIT_FAILURE;
						}
					}
					else {
						MessageBox(hwnd, TEXT("Unknown Error While Generating Task"), TEXT("ERROR"), MB_ICONERROR);
						DeleteFile(TEXT("BatteryProbeTask.xml"));
						return EXIT_FAILURE;
					}
				}
				return 0;
			}

			else if (LOWORD(wParam) == BTN_CHECKSTATUS && HIWORD(wParam) == BN_CLICKED) {
				SYSTEM_POWER_STATUS systemPowerStatus = {};

				bool res = GetSystemPowerStatus(&systemPowerStatus);
				if (!res) {
					MessageBox(hwnd, TEXT("Error Getting System Power Status"), TEXT("ERROR"), MB_ICONERROR);
					return EXIT_FAILURE;
				}

				LPCWSTR acStatus = TEXT("Not Plugged In");
				if (systemPowerStatus.ACLineStatus == '\x1') acStatus = TEXT("Plugged In");

				MessageBox((HWND)lParam, acStatus, TEXT("Result"), MB_ICONINFORMATION);
			}

			else if (LOWORD(wParam) == BTN_ABOUT && HIWORD(wParam) == BN_CLICKED) {
				showAboutMessageBox((HWND)lParam);
			}

			else return DefWindowProc(hwnd, uMsg, wParam, lParam);

			return 0;
		}
		case WM_CLOSE:
			for each (myButton* button in myButton::getButtonList()) {
				delete button;
			}
			DestroyWindow(hwnd);
			return 0;

		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;

		default:
			return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}
}

void showAboutMessageBox(HWND hwnd) {
	WCHAR message[] = TEXT("Battery Prober Help Document\n\n"
		"\n"
		"prober.exe\n"
		"Shows the main GUI window\n\n"
		"prober.exe <arg1> <arg2>\n"
		"Checks the AC power status, if connected, run arg1; if not, run arg2\n\n"
		"prober.exe /h\n"
		"Shows this message\n\n"
		"\n"
		"Note that both arg1 and arg2 HAVE TO BE .exe or .bat files\n\0");

	MessageBox(hwnd, message, TEXT("Information"), MB_ICONQUESTION);
}

bool checkExistingTask() {
	SHELLEXECUTEINFO pExecInfo = {};
	pExecInfo.cbSize = sizeof(pExecInfo);
	pExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
	pExecInfo.lpVerb = TEXT("open");
	pExecInfo.lpFile = TEXT("schtasks.exe");
	pExecInfo.lpParameters = TEXT("/query /tn \"BatteryProber\"");
	pExecInfo.lpDirectory = NULL;
	pExecInfo.nShow = SW_HIDE;

	DWORD exitCode = EXIT_FAILURE;

	ShellExecuteEx(&pExecInfo);
	if (pExecInfo.hProcess) {
		WaitForSingleObject(pExecInfo.hProcess, INFINITE);
		GetExitCodeProcess(pExecInfo.hProcess, &exitCode);
	}
	else return false;

	if (exitCode == EXIT_SUCCESS) return true;
	else return false;
}

bool elevatePrompt(HWND hwnd) {
	int choice = MessageBox(hwnd, TEXT("This Option Requires Administrator Rights\nWould You Like to Restart the Program with Administrator Priviledges?"), TEXT("Administrator Rights Required"), MB_YESNO | MB_ICONWARNING);
	if (choice == IDYES) {
		ShellExecute(NULL, TEXT("runas"), argv[0], TEXT(""), NULL, SW_SHOW);
		return true;
	}
	return false;
}