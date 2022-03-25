#include <stdio.h>
#include <stdbool.h>
#include <windows.h>
#include <afxres.h>
#include <pthread.h>

#include "general.h"
#include "config_res.rh"

char serial_ports[256][256];
int serial_ports_count = 0;
char selected_port[256] = "",
     tested_port[256] = "";

bool save = false;

// reads the serial port list from the registry
void list_ports() {
	serial_ports_count = 0;;
	HKEY serialcommmm;
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "HARDWARE\\DEVICEMAP\\SERIALCOMM",
		0, KEY_READ, &serialcommmm) == ERROR_SUCCESS)
	{
		DWORD c=0;
		RegQueryInfoKey(serialcommmm, 0, 0, 0, 0, 0, 0, &c, 0, 0, 0, 0);
		char name[256], value[256];
		DWORD sizeName, sizeValue, type;
		for (int i = 0; i < c; i++) {
			sizeName = sizeValue = 256;
			if (RegEnumValue(serialcommmm, i, name, &sizeName, 0, &type,
				(LPBYTE) value, &sizeValue) == ERROR_SUCCESS && type == REG_SZ) {
					strncpy(serial_ports[serial_ports_count], value, 256);
					serial_ports_count++;
			}
		}
	}
	RegCloseKey(serialcommmm);
}

// sends every serial port their name as assigned by windows
// this way, led tickers will display their port number
int _identify_ticker() {
	tested_port[0] = 0;
	char text[10] = {0};
	HANDLE hComm;
	for (int i = 0; i < serial_ports_count; i++) {
		ticker_open(serial_ports[i]);
		strncpy(text, serial_ports[i], 9);
		for (int j = strlen(serial_ports[i]); j < 9; j++)
			text[j] = ' ';
		ticker_write(text);
		ticker_close();
	}
	return 0;
}

// _identify_ticker() needs 2 calls because arduino resets on serial connection
// first call disables this behaviour so the second one can work
int identify_ticker() {
	//cout << "Identify..." << endl;
	_identify_ticker();
	Sleep(1000);
	return _identify_ticker();
}

int test_ticker(bool clear) {
	if (!clear && strcmp(selected_port, tested_port))
		test_ticker(true);
	const char *port = clear?tested_port:selected_port;
	char msg[] = "IT WORKS!";
	if (clear) memset(msg, '*', 9);
	ticker_open(port);
	ticker_write(msg);
	ticker_close();
	if (clear) tested_port[0] = 0;
	else strcpy(tested_port, selected_port);
	return 0;
}


void update_port_list(HWND hwnd) {
	list_ports();
	SendDlgItemMessage(hwnd, IDLIST, CB_RESETCONTENT, 0, 0);
	for (int i = 0; i < serial_ports_count; i++) {
		SendDlgItemMessage(hwnd, IDLIST, CB_ADDSTRING, 0,
			(LPARAM)serial_ports[i]);
		if (strcmp(serial_ports[i], selected_port) == 0)
			SendDlgItemMessage(hwnd, IDLIST, CB_SETCURSEL, i, 0);
	}
}

bool identifying = false;
void *clear_identifying(void *data) {
	Sleep(50);
	identifying = false;
}

int CALLBACK config_dlgproc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
		case WM_INITDIALOG:
			strcpy(selected_port, ticker_get_port());
			update_port_list(hwnd);
			return TRUE;
		case WM_COMMAND:
			switch(LOWORD(wParam)) {
				case IDLIST:
					switch(HIWORD(wParam)) {
						case CBN_SELCHANGE: {
							int a = SendDlgItemMessage(hwnd, IDLIST, CB_GETCURSEL, 0, 0);
							strcpy(selected_port, serial_ports[a]);
							// remove IT WORKS! from previous test
							if (strcmp(selected_port, tested_port))
								test_ticker(true);
						}	break;
						default:
							break;
					} break;
				case IDSCAN:
					update_port_list(hwnd);
					break;
				case IDIDENTIFY: 
					if (!identifying) {
						identifying = true;
						HWND hBtn = GetDlgItem(hwnd, IDIDENTIFY);
						EnableWindow(hBtn, FALSE);
						update_port_list(hwnd);
						identify_ticker();
						PostMessage(hwnd, WM_USER, 0, 0);
						EnableWindow(hBtn, TRUE);
						// enable the button back from another thread
						// that way, messages will be handled before the button is back
						pthread_t t;
						pthread_create(&t, 0, clear_identifying, 0);
					} break;
				case IDTEST:
					test_ticker(false);
					break;
				case IDOK:
					ticker_set_port(selected_port);
					ticker_save_config();
					EndDialog(hwnd, IDOK);
					break;
				case IDCANCEL:
					EndDialog(hwnd, IDCANCEL);
					break;
				default:
					break;
			} break;
		case WM_USER:
			EnableWindow(GetDlgItem(hwnd, IDIDENTIFY), TRUE);
			break;
		default:
			return FALSE;
	}
	return TRUE;
}

void config_show_dialog(HWND parent) {
	DialogBox(0, MAKEINTRESOURCE(IDDCONFIG), parent, config_dlgproc);
	test_ticker(true);
}

