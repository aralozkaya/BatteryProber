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
#include <Windows.h>
#include <vector>
#pragma warning(disable : 4996)

LRESULT CALLBACK mainWindowProc(HWND, UINT, WPARAM, LPARAM);
void showAboutMessageBox(HWND hwnd);

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd) {
	LPCWSTR lpwCmdLine = GetCommandLine();
	int argc;

	LPWSTR *argv = CommandLineToArgvW(lpwCmdLine, &argc);

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

			//Setting height || width to -1 sets them automatically using text font dimensions
			myButton ScheduleButton(TEXT("**NOT IMPLEMENTED YET**"), -1, 300, hwnd, (HMENU) BTN_SCHEDULE);
			myButton checkACButton(TEXT("Check AC Status"), -1, 300, hwnd, (HMENU) BTN_CHECKSTATUS);
			myButton aboutButton(TEXT("About the Usage"), -1, 300, hwnd, (HMENU) BTN_ABOUT);

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

		case WM_COMMAND:
			if (LOWORD(wParam) == BTN_CHECKSTATUS && HIWORD(wParam) == BN_CLICKED) {
				SYSTEM_POWER_STATUS systemPowerStatus = {};

				bool res = GetSystemPowerStatus(&systemPowerStatus);
				if (!res) {
					MessageBox(NULL, TEXT("Error Getting System Power Status"), TEXT("ERROR"), MB_ICONERROR);
					return EXIT_FAILURE;
				}

				LPCWSTR acStatus = TEXT("Not Plugged In");
				if (systemPowerStatus.ACLineStatus == '\x1') acStatus = TEXT("Plugged In");
				
				MessageBox((HWND) lParam, acStatus, TEXT("Result"), MB_ICONINFORMATION);
			}

			else if (LOWORD(wParam) == BTN_ABOUT && HIWORD(wParam) == BN_CLICKED) {
				showAboutMessageBox((HWND)lParam);
			}

			else return DefWindowProc(hwnd, uMsg, wParam, lParam);

			return 0;

		case WM_CLOSE:
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