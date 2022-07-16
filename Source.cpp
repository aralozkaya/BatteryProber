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

#include <Windows.h>
#pragma warning(disable : 4996)

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd) {
	LPCWSTR lpwCmdLine = GetCommandLine();
	int argc;

	LPWSTR *argv = CommandLineToArgvW(lpwCmdLine, &argc);

	SYSTEM_POWER_STATUS systemPowerStatus = {};
	bool res = GetSystemPowerStatus(&systemPowerStatus);

	if (!res) {
		MessageBox(NULL, TEXT("Error Getting System Power Status"), TEXT("ERROR"), MB_ICONERROR);
		return EXIT_FAILURE;
	}
	
	switch (argc) {
		case 1: 
		{
			LPCWSTR acStatus = TEXT("Not Plugged In");
			if (systemPowerStatus.ACLineStatus == '\x1') acStatus = TEXT("Plugged In");
			MessageBox(NULL, acStatus, TEXT("Result"), MB_ICONINFORMATION);
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

		default:
			WCHAR message[] = TEXT("Battery Prober Help Document\n\n"
				"\n"
				"prober.exe\n"
				"Shows a window with the current battery status(for testing purposes)\n\n"
				"prober.exe <arg1> <arg2>\n"
				"Checks the AC power status, if connected, run arg1; if not, run arg2\n\n"
				"\n"
				"Note that both arg1 and arg2 HAVE TO BE .exe or .bat files");

			MessageBox(NULL, message, TEXT("Information"), MB_ICONQUESTION);

			return EXIT_FAILURE;
	}

	LocalFree(argv);
	return EXIT_SUCCESS;
}