#ifndef __GAMELIST_H__
#define __GAMELIST_H__

struct Game {
	char title[128];
	char window_title[128];
	char module_name[32];
	int memory_offset;
};

extern const struct Game gamelist[];

#endif//__GAMELIST_H__
