#define _GNU_SOURCE
#include "config.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/stat.h>
#include <limits.h>

static char g_config_file_path[PATH_MAX] = {0};
static char g_logs_dir_path[PATH_MAX] = {0};

const char *config_get_file_path(void) {
    if (g_config_file_path[0] != '\0') {
        return g_config_file_path;
    }

    const char *xdg_config = getenv("XDG_CONFIG_HOME");
    if (xdg_config && *xdg_config) {
        snprintf(g_config_file_path, sizeof(g_config_file_path), "%s/winerunner/games.conf", xdg_config);
    } else {
        const char *home = getenv("HOME");
        if (!home) home = "";
        snprintf(g_config_file_path, sizeof(g_config_file_path), "%s/.config/winerunner/games.conf", home);
    }
    return g_config_file_path;
}

const char *config_get_logs_dir(void) {
    if (g_logs_dir_path[0] != '\0') {
        return g_logs_dir_path;
    }

    const char *xdg_state = getenv("XDG_STATE_HOME");
    if (xdg_state && *xdg_state) {
        snprintf(g_logs_dir_path, sizeof(g_logs_dir_path), "%s/winerunner/logs", xdg_state);
    } else {
        const char *home = getenv("HOME");
        if (!home) home = "";
        snprintf(g_logs_dir_path, sizeof(g_logs_dir_path), "%s/.local/state/winerunner/logs", home);
    }
    return g_logs_dir_path;
}

int config_load_all(GameConfig **out_games, size_t *out_count) {
    if (!out_games || !out_count) return -1;

    *out_games = NULL;
    *out_count = 0;

    const char *filepath = config_get_file_path();
    FILE *fp = fopen(filepath, "r");
    if (!fp) {
        /* Config file doesn't exist yet, return empty list cleanly */
        return 0;
    }

    size_t capacity = 8;
    GameConfig *games = malloc(capacity * sizeof(GameConfig));
    if (!games) {
        fclose(fp);
        return -1;
    }

    size_t count = 0;
    GameConfig *current = NULL;
    char line[2048];

    while (fgets(line, sizeof(line), fp)) {
        trim_whitespace(line);

        /* Skip comments and empty lines */
        if (*line == '\0' || *line == '#' || *line == ';') {
            continue;
        }

        /* Section header: [game_name] */
        if (line[0] == '[' && line[strlen(line) - 1] == ']') {
            line[strlen(line) - 1] = '\0';
            char *sec_name = line + 1;
            trim_whitespace(sec_name);

            if (*sec_name == '\0') continue;

            if (count >= capacity) {
                capacity *= 2;
                GameConfig *tmp = realloc(games, capacity * sizeof(GameConfig));
                if (!tmp) {
                    free(games);
                    fclose(fp);
                    return -1;
                }
                games = tmp;
            }

            current = &games[count++];
            memset(current, 0, sizeof(GameConfig));
            snprintf(current->name, sizeof(current->name), "%.*s",
                     (int)(sizeof(current->name) - 1), sec_name);
            snprintf(current->desktop_name, sizeof(current->desktop_name), "%.*s",
                     (int)(sizeof(current->desktop_name) - 1), sec_name);
            snprintf(current->resolution, sizeof(current->resolution), "1920x1080");
            current->virtual_desktop = true;
            continue;
        }

        /* Key = Value */
        if (!current) continue;

        char *eq = strchr(line, '=');
        if (!eq) continue;

        *eq = '\0';
        char *key = line;
        char *val = eq + 1;
        trim_whitespace(key);
        trim_whitespace(val);

        if (strcasecmp(key, "exe") == 0 || strcasecmp(key, "path") == 0) {
            strncpy(current->exe_path, val, sizeof(current->exe_path) - 1);
            if (current->workdir[0] == '\0') {
                char *dir = get_directory_of_file(val);
                if (dir) {
                    strncpy(current->workdir, dir, sizeof(current->workdir) - 1);
                    free(dir);
                }
            }
        } else if (strcasecmp(key, "workdir") == 0 || strcasecmp(key, "cwd") == 0) {
            strncpy(current->workdir, val, sizeof(current->workdir) - 1);
        } else if (strcasecmp(key, "resolution") == 0 || strcasecmp(key, "res") == 0) {
            strncpy(current->resolution, val, sizeof(current->resolution) - 1);
        } else if (strcasecmp(key, "desktop_name") == 0) {
            strncpy(current->desktop_name, val, sizeof(current->desktop_name) - 1);
        } else if (strcasecmp(key, "virtual_desktop") == 0 || strcasecmp(key, "explorer") == 0) {
            if (strcasecmp(val, "true") == 0 || strcasecmp(val, "1") == 0 ||
                strcasecmp(val, "yes") == 0 || strcasecmp(val, "on") == 0) {
                current->virtual_desktop = true;
            } else {
                current->virtual_desktop = false;
            }
        } else if (strcasecmp(key, "wineprefix") == 0 || strcasecmp(key, "prefix") == 0) {
            strncpy(current->wineprefix, val, sizeof(current->wineprefix) - 1);
        } else if (strcasecmp(key, "env") == 0 || strcasecmp(key, "env_vars") == 0) {
            strncpy(current->env_vars, val, sizeof(current->env_vars) - 1);
        } else if (strcasecmp(key, "extra_args") == 0 || strcasecmp(key, "args") == 0) {
            strncpy(current->extra_args, val, sizeof(current->extra_args) - 1);
        }
    }

    fclose(fp);

    *out_games = games;
    *out_count = count;
    return 0;
}

int config_save_all(const GameConfig *games, size_t count) {
    const char *filepath = config_get_file_path();

    /* Ensure parent directory exists */
    char *config_dir = get_directory_of_file(filepath);
    if (config_dir) {
        ensure_dir(config_dir);
        free(config_dir);
    }

    char tmp_path[PATH_MAX];
    snprintf(tmp_path, sizeof(tmp_path), "%s.tmp.%d", filepath, getpid());

    FILE *fp = fopen(tmp_path, "w");
    if (!fp) {
        return -1;
    }

    fprintf(fp, "# winerunner configuration file\n");
    fprintf(fp, "# Managed automatically by winerunner. You can also edit this file directly.\n\n");

    for (size_t i = 0; i < count; i++) {
        const GameConfig *g = &games[i];
        fprintf(fp, "[%s]\n", g->name);
        fprintf(fp, "exe = %s\n", g->exe_path);
        fprintf(fp, "workdir = %s\n", g->workdir);
        fprintf(fp, "resolution = %s\n", g->resolution[0] ? g->resolution : "1920x1080");
        fprintf(fp, "desktop_name = %s\n", g->desktop_name[0] ? g->desktop_name : g->name);
        fprintf(fp, "virtual_desktop = %s\n", g->virtual_desktop ? "true" : "false");
        fprintf(fp, "wineprefix = %s\n", g->wineprefix);
        fprintf(fp, "env = %s\n", g->env_vars);
        fprintf(fp, "extra_args = %s\n\n", g->extra_args);
    }

    fclose(fp);

    if (rename(tmp_path, filepath) != 0) {
        unlink(tmp_path);
        return -1;
    }

    return 0;
}

int config_find(const char *name, GameConfig *out_game) {
    if (!name || !out_game) return -1;

    GameConfig *games = NULL;
    size_t count = 0;
    if (config_load_all(&games, &count) != 0) {
        return -1;
    }

    for (size_t i = 0; i < count; i++) {
        if (strcasecmp(games[i].name, name) == 0) {
            *out_game = games[i];
            config_free_list(games);
            return 0;
        }
    }

    config_free_list(games);
    return -1; /* Not found */
}

int config_add_or_update(const GameConfig *game) {
    if (!game || game->name[0] == '\0') return -1;

    GameConfig *games = NULL;
    size_t count = 0;
    config_load_all(&games, &count);

    /* Check if already exists */
    for (size_t i = 0; i < count; i++) {
        if (strcasecmp(games[i].name, game->name) == 0) {
            games[i] = *game;
            int ret = config_save_all(games, count);
            config_free_list(games);
            return ret;
        }
    }

    /* Add new game */
    GameConfig *new_games = realloc(games, (count + 1) * sizeof(GameConfig));
    if (!new_games) {
        config_free_list(games);
        return -1;
    }

    new_games[count] = *game;
    int ret = config_save_all(new_games, count + 1);
    config_free_list(new_games);
    return ret;
}

int config_remove(const char *name) {
    if (!name || *name == '\0') return -1;

    GameConfig *games = NULL;
    size_t count = 0;
    if (config_load_all(&games, &count) != 0 || count == 0) {
        config_free_list(games);
        return -1;
    }

    ssize_t target_idx = -1;
    for (size_t i = 0; i < count; i++) {
        if (strcasecmp(games[i].name, name) == 0) {
            target_idx = (ssize_t)i;
            break;
        }
    }

    if (target_idx == -1) {
        config_free_list(games);
        return -1; /* Not found */
    }

    for (size_t i = (size_t)target_idx; i < count - 1; i++) {
        games[i] = games[i + 1];
    }

    int ret = config_save_all(games, count - 1);
    config_free_list(games);
    return ret;
}

void config_free_list(GameConfig *games) {
    if (games) {
        free(games);
    }
}
