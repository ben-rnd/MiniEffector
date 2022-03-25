#include <stdio.h>
#include <stdbool.h>
#include <windows.h>

static char ticker_com_port[256] = "",
            ticker_game_version[256] = "";
static char config_dir[512] = "",
            config_file[512] = "";
static DCB dcb = {0};
static HANDLE hComm;

const char *ticker_get_port() {
	return ticker_com_port;
}
void ticker_set_port(const char* port) {
	strncpy(ticker_com_port, port, 256);
}

const char *ticker_get_game() {
	return ticker_game_version;
}
void ticker_set_game(const char* game) {
	strncpy(ticker_game_version, game, 256);
}

const char *ticker_get_config_dir() {
	return config_dir;
}

int ticker_load_config() {
	if (!config_file[0]) return 1;
	FILE *f = fopen(config_file, "r");
	if (!f) return 2;
	char name[256] = "", value[256] = "", *s = 0;
	int c = fgetc(f), l = 0;
	bool v = false;
	while (c != EOF) {
		if (c == '\n') {
			if (strcmp(name,"ticker_com_port:") == 0)
				strcpy(ticker_com_port, value);
			if (strcmp(name,"ticker_game_version:") == 0)
				strcpy(ticker_game_version, value);
			name[0] = value[0] = 0;
			v = false;
		} else if (!v && c == ' ')  {
			v = true;
		} else {
			s = v ? value : name;
			l = strlen(s);
			if (l < 255) {
				s[l] = c;
				s[l+1] = 0;
			}
		}
		c = fgetc(f);
	}
	fclose(f);
	return 0;
}

int ticker_save_config() {
	if (!config_file[0]) return 1;
	FILE *f = fopen(config_file, "w");
	if (!f) return 2;
	fprintf(f, "ticker_com_port: %s\n", ticker_com_port);
	fprintf(f, "ticker_game_version: %s\n", ticker_game_version);
	fclose(f);
	return 0;
}


int ticker_init() {
	const char *appdata = getenv("APPDATA");
	if (strlen(appdata) <= 480) {
		strcpy(config_dir, appdata);
		strcat(config_dir, "\\ledticker");
		strcpy(config_file, config_dir);
		strcat(config_file, "\\config.txt");
		CreateDirectory(config_dir, 0);
	}

	dcb.DCBlength = sizeof(dcb);
	if (!BuildCommDCB("115200,n,8,1", &dcb)) return 1;
	dcb.fDtrControl = DTR_CONTROL_DISABLE;

	return 0;
}

int ticker_open(const char *com_port) {
	if (com_port == 0)
		com_port = ticker_com_port;
	char com_path[260] = "\\\\.\\";
	strncat(com_path, com_port, 256);
	hComm = CreateFile(com_path, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0);
	if (hComm == INVALID_HANDLE_VALUE) return 1;
	if (!SetCommState(hComm, &dcb)) return 2;
	return 0;
}

int ticker_write(const char *text) {
	char _text[11] = {0xaf, 0};
	strncat(_text, text, 9);
	DWORD c;
	return !WriteFile(hComm, _text, 11, &c, 0);
}

int ticker_close() {
	CloseHandle(hComm);
	return 0;
}

