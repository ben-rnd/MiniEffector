#include <windows.h>
#include <commctrl.h>
#include "general.h"
#include "config_tool.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
	LPSTR lpCmdLine, int nCmdShow)
{
	// init common controls
	// needed for windows xp (error 0x583)
	INITCOMMONCONTROLSEX icc;
	icc.dwSize = sizeof(icc);
	icc.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&icc);

	ticker_init();
	ticker_load_config();

	config_show_dialog(0);

	return 0;
}
