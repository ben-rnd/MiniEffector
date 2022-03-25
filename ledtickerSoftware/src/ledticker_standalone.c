#include <stdio.h>
#include <stdbool.h>
#include <windows.h>
#include <commctrl.h>
#include <psapi.h>
#include "general.h"
#include "standalone_res.rh"
#include "gamelist.h"
#include "config_tool.h"

int selected_game = -1;

void update_ui(HWND hwnd) {
	const char* port = ticker_get_port();
	if (port[0])
		SetDlgItemText(hwnd, IDC_PORT, port);
	else
		SetDlgItemText(hwnd, IDC_PORT, "(unset)");
	EnableWindow(GetDlgItem(hwnd, IDC_GAMELIST), port[0]);
	EnableWindow(GetDlgItem(hwnd, IDOK), port[0] && selected_game >= 0);
}

int CALLBACK DlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
		case WM_INITDIALOG: {
			for (int i = 0; gamelist[i].memory_offset != 0; i++) {
				SendDlgItemMessage(hwnd, IDC_GAMELIST, CB_ADDSTRING, 0, (LPARAM)gamelist[i].title);
			}
			SendDlgItemMessage(hwnd, IDC_GAMELIST, CB_SETCURSEL, selected_game, 0);
			update_ui(hwnd);
		}	break;
		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDC_GAMELIST:
					selected_game = SendDlgItemMessage(hwnd, IDC_GAMELIST, CB_GETCURSEL, 0, 0);
					update_ui(hwnd);
					break;
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

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {

	printf(
	"---------------------------\n"
	"-                         -\n"
	"-     IIDX LED TICKER     -\n"
	"-                         -\n"
	"---------------------------\n\n"
	);

	// init common controls
	// needed for windows xp (error 0x583)
	INITCOMMONCONTROLSEX icc;
	icc.dwSize = sizeof(icc);
	icc.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&icc);

	char games_path[512] = "";
	FILE *f;
	bool gui = false;

	ticker_init();
	ticker_load_config();
	// either use game specified in argument or in config
	const char* game = lpCmdLine[0]?lpCmdLine : ticker_get_game();
	for (int i = 0; gamelist[i].memory_offset != 0; i++) {
		if (strcmp(game, gamelist[i].title)==0) {
			selected_game = i;
		}
	}

	if (lpCmdLine[0]) {
		if (selected_game < 0) {
			printf(
				"Usage: ledticker_standalone.exe [<game>]\n\n"
				"When specified, <game> must be one of the following:\n");
			for (int i = 0; gamelist[i].memory_offset != 0; i++) {
				printf(" * %s\n", gamelist[i].title);
			}
			printf("As an example, try \"ledticker_standalone.exe %s\"\n", gamelist[0].title);
			printf("\nPress return to close this window...\n");
			getchar();
			return 1;
		}
		else if (!ticker_get_port()[0]) {
			printf(
				"error: serial port is not set\n"
				"To configure it, please run this program without specifying the game\n");
			printf("\nPress return to close this window...\n");
			getchar();
			return 1;
		}
	}

	printf("Ticker is on %s\n", ticker_get_port());

	bool reconnect = true;
	while (true) {
		reconnect = false;
		printf("\nConnecting to ticker...");
		fflush(stdout);
		while(ticker_open(0)) Sleep(100);
		printf(" OK\n");
		ticker_write("*********");
		
		Sleep(2000);

		printf("Waiting for IIDX...\n");
		fflush(stdout);


		bool searching = true;
		HANDLE phandle;
		void *address;
		while (searching) {
			DWORD modcount = 0;
			HWND hwnd;
			DWORD pid;
			HMODULE modlist[1024];
			MODULEINFO module_info;
			char modname[32];
			int m, n, modname_l;
			Sleep(100);
			for(int j = 0; gamelist[j].memory_offset != 0; j++){
				const struct Game *game = &gamelist[j];
				const char *wanted = game->module_name;
				int wanted_l = strlen(wanted);

				hwnd = FindWindow(0, game->window_title);
				if (!hwnd) continue;
				GetWindowThreadProcessId(hwnd, &pid);
				phandle = OpenProcess(0x1f0fff, false, pid);
				EnumProcessModules(phandle, modlist, sizeof(modlist), &modcount);
				n = (modcount < sizeof(modlist) ? modcount:sizeof(modlist)) / sizeof(HMODULE);
				for (int i = 0; i < n; i++) {
					modname_l = GetModuleBaseName(phandle, modlist[i], modname, 32);
					if (modname_l == wanted_l && strncmp(modname, wanted, 32) == 0) {
						GetModuleInformation(phandle, modlist[i], &module_info, sizeof(MODULEINFO));
						address = module_info.lpBaseOfDll + game->memory_offset;
						searching = false;
						printf("Found %s... OK\n", game->title);
						break;
					}
				}
				if(searching == false){
					break;
				}
			}
			Sleep(1000);
		}

		char text[10] = {0}, text_o[10] = {0};
		while (ReadProcessMemory(phandle, address, text, 9, 0)) {
			if (strcmp(text, text_o)) {
				//printf("%s END OF LINE\n ", text);
				if (ticker_write(text)) {
					reconnect = true;
					ticker_write("*********");
					break;
				}
				strcpy(text_o, text);
			}
			Sleep(50);
		}

		ticker_close();
		break;
	}

	return 0;
}
