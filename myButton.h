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

#pragma once
#include <Windows.h>
#include <vector>
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
		bTextLen = (int)wcslen(bText);
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
		int index = (int)(it - buttonList.begin());
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

	BOOL setPos(int x, int y, int width, int height) {
		BOOL res = SetWindowPos(hWnd, HWND_TOP, x, y, width, height, SWP_FRAMECHANGED | SWP_NOZORDER | SWP_SHOWWINDOW | SWP_NOSIZE);
		if (res == TRUE) {
			this->x = x;
			this->y = y;
			this->width = width;
			this->height = height;
		}

		return res;
	};

	BOOL changeText(LPCTSTR newText) {
		this->bText = newText;
		DestroyWindow(this->hWnd);
		this->hWnd = CreateWindow(
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
		return RedrawWindow(hParent, NULL, NULL, RDW_INVALIDATE);
	}

	static std::vector<myButton*> getButtonList() { return buttonList; };
	static myButton* findButtonByHMENU(HMENU hMenu) {
		for each (myButton* button in buttonList){
			if (button->getHMENU() == hMenu) {
				return button;
			}
		}
		return NULL;
	}
};

std::vector<myButton*> myButton::buttonList = {};
HFONT myButton::guiFont = NULL;
NONCLIENTMETRICS myButton::metrics = {};
bool myButton::initialized = false;