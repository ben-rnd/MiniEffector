#include <stdio.h>
#include <stdbool.h>
#include <windows.h>
#include <psapi.h>
#include <jansson.h>
#include "general.h"
#include "standalone_res.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	char games_path[512] = "";
	FILE *f;

	json_error_t error;
	json_t *games_json, *games_list;

	ticker_init();
	ticker_load_config();

	snprintf(games_path, 512, "%s\\games.json", ticker_get_config_dir());
	f = fopen(games_path, "r");
	// f does not exist, 
	if (!f) {
		f = fopen(games_path, "w");
		if (!f) return 1;
		HMODULE handle = GetModuleHandle(0);
		HRSRC rc = FindResource(handle, MAKEINTRESOURCE(ID_GAMES_JSON), MAKEINTRESOURCE(TEXTFILE));
		HGLOBAL rcData = LoadResource(handle, rc);
		DWORD size = SizeofResource(handle, rc);
		const char *data = (const char*)LockResource(rcData);
		fwrite(data, sizeof(char), size, f);
		fclose(f);
		f = fopen(games_path, "r");
		if (!f) return 2;
	}
	games_json = json_loadf(f, 0, &error);
	fclose(f);

	if (!games_json) {
		fprintf(stderr, "error: %s line %d:\n\t%s\n", games_path, error.line, error.text);
		return 1;
	}
	
	games_list = json_object_get(games_json, "gameList");
	for (int i = 0; i < json_array_size(games_list); i++) {
		json_t *game = json_array_get(games_list, i);
		printf("beatmania IIDX %" JSON_INTEGER_FORMAT " %s\n",
			json_integer_value(json_object_get(game, "gameIndex")),
			json_string_value(json_object_get(game,"gameTitle")));
	}

	json_t *game = json_array_get(games_list, 0);
	printf("Running for %s\n", json_string_value(json_object_get(game, "gameTitle")));

	char *window_title, *module_name;
	const char *_window_title = json_string_value(json_object_get(game, "windowTitle")),
	           *_module_name = json_string_value(json_object_get(game, "moduleName"));
	window_title = malloc(sizeof(char) * strlen(_window_title));
	module_name = malloc(sizeof(char) * strlen(_module_name));
	strcpy(window_title, _window_title);
	strcpy(module_name, _module_name);

	json_int_t memory_offset = json_integer_value(json_object_get(game, "memoryOffset"));

	printf("decref\n");
	json_decref(games_list);
	printf("ok\n");

	while (1) {
		printf("Connecting to ticker...");
		fflush(stdout);
		while(ticker_open(0)) Sleep(50);
		printf(" OK\n");
		Sleep(2000);

		printf("Waiting for game...");
		fflush(stdout);

		HWND hwnd;
		DWORD pid;
		HANDLE phandle;
		HMODULE modlist[256];
		DWORD modcount;
		do {
			modcount = 0;
			hwnd = FindWindow(0, window_title);
			if (!hwnd) continue;
			GetWindowThreadProcessId(hwnd, &pid);
			phandle = OpenProcess(0x1f0fff, false, pid);
			EnumProcessModules(phandle, modlist, sizeof(modlist), &modcount);
			Sleep(50);
		} while (modcount == 0);
		printf("Found %d modules\n", modcount/sizeof(HMODULE));

		ticker_close();
	}

	free(window_title);
	free(module_name);

	return 0;
}
