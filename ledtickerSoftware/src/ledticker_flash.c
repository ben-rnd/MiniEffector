#include <stdio.h>
#include <stdbool.h>
#include <windows.h>
#include <commctrl.h>
#include "general.h"
#include "flash_res.rh"
#include "config_tool.h"

void update_ui(HWND hwnd) {
	const char* port = ticker_get_port();
	if (port[0])
		SetDlgItemText(hwnd, IDC_PORT, port);
	else
		SetDlgItemText(hwnd, IDC_PORT, "(unset)");
	EnableWindow(GetDlgItem(hwnd, IDOK), port[0]);
}


int CALLBACK DlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
		case WM_INITDIALOG: {
			update_ui(hwnd);
		}	break;
		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDC_EDIT:
					config_show_dialog(hwnd);
					update_ui(hwnd);
					break;
				case IDOK:
					EndDialog(hwnd, IDOK);
					break;
				case IDCANCEL:
					EndDialog(hwnd, IDCANCEL);
					break;
				default:
					break;
			}
			break;
		default:
			return FALSE;
			break;
	}
	return TRUE;
}

int end(int r) {
	printf("Press return to close this window...\n");
	getchar();
	return r;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {

	printf(
	"---------------------------\n"
	"-     IIDX LED TICKER     -\n"
	"-    flashing software    -\n"
	"-      firmware V0.2      -\n"
	"---------------------------\n\n"
	);

	// init common controls
	// needed for windows xp (error 0x583)
	INITCOMMONCONTROLSEX icc;
	icc.dwSize = sizeof(icc);
	icc.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&icc);

	ticker_init();
	ticker_load_config();

	if (DialogBox(0, MAKEINTRESOURCE(IDD_FLASH), 0, DlgProc) != IDOK)
		return 0;

	const char* port = ticker_get_port();
	if (!port[0]) {
		printf("Error: no port selected !\n");
		return end(1);
	}

	printf("Ticker is on %s\n", port);
	if (ticker_open(0)) {
		printf("Error: could not find ticker\n");
		return end(1);
	}
	ticker_close();

	char command[512] = "avrdude -p atmega2560 -c wiring -b 115200 -D -U flash:w:firmware02.hex:i -P ";
	strcat(command, port);

	printf("%s\n", command);

	system(command);

	return end(0);
}
