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
#include <Windows.h>
#include <vector>
#pragma warning(disable : 4996)

class myButton {
	private:
		static std::vector<myButton*> buttonList;
		static NONCLIENTMETRICS metrics;
		static HFONT guiFont;
		static bool initialized;
		
		const HWND hParent;
		const HINSTANCE hInstance;
		const DWORD dwStyle = WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON;

		int width;
		int height;
		int x;
		int y;
		HWND hWnd;
		HDC hdc;
		LPCTSTR bText;
		HMENU hMenu;
		int bTextLen;
		SIZE bTextSize = {};

	public:
		myButton(LPCTSTR bText, int height, int width, HWND hParent, HMENU hMenu) 
			: height(height), width(width), bText(bText), hParent(hParent), hMenu(hMenu), hInstance((HINSTANCE)GetWindowLongPtr(hParent, GWLP_HINSTANCE))
		{
			if (!initialized) {
				metrics.cbSize = sizeof(metrics);

				SystemParametersInfo(SPI_GETNONCLIENTMETRICS, metrics.cbSize, &metrics, 0);
				guiFont = CreateFontIndirect(&metrics.lfCaptionFont);

				initialized = true;
			}

			hdc = GetDC(hParent);
			bTextSize = {};
			bTextLen = wcslen(bText);
			GetTextExtentPoint32(hdc, bText, bTextLen, &bTextSize);

			if (height == -1) {
				this->height = bTextSize.cy * 2;
			}

			if (width == -1) {
				this->width = bTextSize.cx * 2;
			}
			 
			 buttonList.push_back(this);
		}

		~myButton() {
			std::vector<myButton*>::iterator it;
			it = find(buttonList.begin(), buttonList.end(), this);
			int index = it - buttonList.begin();
			buttonList.erase(buttonList.begin() + index);
			if (buttonList.size() == 0) {
				initialized = false;
				FreeResource(guiFont);
			}
		}

		void create(int x, int y) {
			this->x = x;
			this->y = y;

			hWnd = CreateWindow(
				L"BUTTON",
				bText,
				dwStyle,
				x,
				y,
				width,
				height,
				hParent,
				hMenu,
				hInstance,
				NULL);

			SendMessage(hWnd, WM_SETFONT, (LPARAM)guiFont, true);
		}

		LPCTSTR getText() const { return this->bText; };
		HMENU getHMENU() const { return this->hMenu; };
		int getHeight() const { return this->height; };
		int getWidth() const { return this->width; };
		SIZE getBTextSize() const { return this->bTextSize; };
		int leftPos() const { return this->x; };
		int rightPos() const { return this->x + this->width; };
		int topPos() const { return this->y; };
		int bottomPos() const { return this->y + this->height; };

		bool setPos(int x, int y, int width, int height) {
			BOOL res = SetWindowPos(hWnd, HWND_TOP, x, y, width, height, SWP_FRAMECHANGED | SWP_NOZORDER | SWP_SHOWWINDOW | SWP_NOSIZE);
			if (res == TRUE) {
				this->x = x;
				this->y = y;
				this->width = width;
				this->height = height;
			}
			
			return res;
		};

		static std::vector<myButton*> getButtonList() { return buttonList; };
};

std::vector<myButton*> myButton::buttonList = {};
HFONT myButton::guiFont = NULL;
NONCLIENTMETRICS myButton::metrics = {};
bool myButton::initialized = false;

LRESULT CALLBACK mainWindowProc(HWND, UINT, WPARAM, LPARAM);

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
			AttachConsole(ATTACH_PARENT_PROCESS);
			HANDLE consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
			WCHAR message[] = TEXT("\nBattery Prober Help Document\n\n"
				"\n"
				"prober.exe\n"
				"Shows a window with the current battery status(for testing purposes)\n\n"
				"prober.exe <arg1> <arg2>\n"
				"Checks the AC power status, if connected, run arg1; if not, run arg2\n\n"
				"\n"
				"Note that both arg1 and arg2 HAVE TO BE .exe or .bat files\n\0");
			size_t messageLen = wcslen(message);
			WriteConsole(consoleHandle, message, messageLen, NULL, NULL);
			FreeConsole();
			LocalFree(consoleHandle);
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
				WCHAR message[] = TEXT("Battery Prober Help Document\n\n"
					"\n"
					"prober.exe\n"
					"Shows a window with the current battery status(for testing purposes)\n\n"
					"prober.exe <arg1> <arg2>\n"
					"Checks the AC power status, if connected, run arg1; if not, run arg2\n\n"
					"\n"
					"Note that both arg1 and arg2 HAVE TO BE .exe or .bat files");

				MessageBox((HWND) lParam, message, TEXT("Information"), MB_ICONQUESTION);
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