#ifndef WINERUNNER_CONFIG_H
#define WINERUNNER_CONFIG_H

#include <stdbool.h>
#include <stddef.h>

#define MAX_GAME_NAME_LEN    64
#define MAX_PATH_LEN         1024
#define MAX_RES_LEN          32
#define MAX_ENV_LEN          1024
#define MAX_ARGS_LEN         1024

typedef struct {
    char name[MAX_GAME_NAME_LEN];          /* Unique alias, e.g. "nfsmw" */
    char exe_path[MAX_PATH_LEN];          /* Path to .exe */
    char workdir[MAX_PATH_LEN];           /* Working directory (usually exe folder) */
    char resolution[MAX_RES_LEN];         /* e.g. "1920x1080" */
    char desktop_name[MAX_GAME_NAME_LEN]; /* Virtual desktop title, e.g. "speed" */
    bool virtual_desktop;                 /* Use wine explorer /desktop=... */
    char wineprefix[MAX_PATH_LEN];        /* Custom WINEPREFIX (optional) */
    char env_vars[MAX_ENV_LEN];           /* Extra env vars (e.g. "DXVK_HUD=fps") */
    char extra_args[MAX_ARGS_LEN];        /* Extra runtime arguments */
} GameConfig;

/* Path helpers */
const char *config_get_file_path(void);
const char *config_get_logs_dir(void);

/* Database / File operations */
int config_load_all(GameConfig **out_games, size_t *out_count);
int config_save_all(const GameConfig *games, size_t count);
int config_find(const char *name, GameConfig *out_game);
int config_add_or_update(const GameConfig *game);
int config_remove(const char *name);
void config_free_list(GameConfig *games);

#endif /* WINERUNNER_CONFIG_H */
