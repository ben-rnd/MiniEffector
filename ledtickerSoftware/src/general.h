#ifndef __CONFIG_H__
#define __CONFIG_H__

const char *ticker_get_port();
void ticker_set_port(const char *port);
const char *ticker_get_game();
void ticker_set_game(const char *game);

const char *ticker_get_config_dir();
int ticker_load_config();
int ticker_save_config();

int ticker_init();
int ticker_open(const char *com_port); // use 0 for default
int ticker_write(const char *text);
int ticker_close();

#endif//__CONFIG_H__
