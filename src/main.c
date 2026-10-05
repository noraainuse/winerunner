#define _GNU_SOURCE
#include "version.h"
#include "utils.h"
#include "config.h"
#include "runner.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <limits.h>

static void print_banner(void) {
    printf("%s%s   🍷 %s v%s%s\n", ANSI_BOLD, ANSI_BRIGHT_MAGENTA, WINERUNNER_NAME, WINERUNNER_VERSION, ANSI_RESET);
    printf("%s   %s%s\n\n", ANSI_DIM, WINERUNNER_DESCRIPTION, ANSI_RESET);
}

static void print_help(const char *prog_name) {
    print_banner();
    printf("%sUSAGE:%s\n", ANSI_BOLD, ANSI_RESET);
    printf("  %s%s%s %s<game-name>%s [run-options]\n", ANSI_CYAN, prog_name, ANSI_RESET, ANSI_BOLD, ANSI_RESET);
    printf("  %s%s%s %s<command>%s [options]\n\n", ANSI_CYAN, prog_name, ANSI_RESET, ANSI_BOLD, ANSI_RESET);

    printf("%sCORE COMMANDS:%s\n", ANSI_BOLD, ANSI_RESET);
    printf("  %sadd%s [name] [path] [flags]  Register a new game (interactive if args omitted)\n", ANSI_GREEN, ANSI_RESET);
    printf("  %sremove%s, %srm%s <game-name>     Remove a registered game\n", ANSI_GREEN, ANSI_RESET, ANSI_GREEN, ANSI_RESET);
    printf("  %slist%s, %sls%s                   List all registered games and running status\n", ANSI_GREEN, ANSI_RESET, ANSI_GREEN, ANSI_RESET);
    printf("  %sinfo%s <game-name>           Show detailed configuration for a game\n", ANSI_GREEN, ANSI_RESET);
    printf("  %slogs%s <game-name> [-n N]    View log output for a game\n", ANSI_GREEN, ANSI_RESET);
    printf("  %skill%s, %sstop%s <game-name>     Terminate running instance of a game\n", ANSI_GREEN, ANSI_RESET, ANSI_GREEN, ANSI_RESET);
    printf("  %sconfig%s                     Show config file path or open in editor\n", ANSI_GREEN, ANSI_RESET);
    printf("  %shelp%s, %s-h%s, %s--help%s           Show this help message\n", ANSI_GREEN, ANSI_RESET, ANSI_GREEN, ANSI_RESET, ANSI_GREEN, ANSI_RESET);
    printf("  %sversion%s, %s-v%s                Show version information\n\n", ANSI_GREEN, ANSI_RESET, ANSI_GREEN, ANSI_RESET);

    printf("%sRUN OPTIONS (for '%s <game-name>'):%s\n", ANSI_BOLD, prog_name, ANSI_RESET);
    printf("  %s-f%s, %s--foreground%s         Run in foreground (stream wine output directly)\n", ANSI_YELLOW, ANSI_RESET, ANSI_YELLOW, ANSI_RESET);
    printf("  %s-d%s, %s--detached%s           Run detached in background (default)\n", ANSI_YELLOW, ANSI_RESET, ANSI_YELLOW, ANSI_RESET);
    printf("  %s-r%s, %s--res <WxH>%s          Override virtual desktop resolution (e.g. 1920x1080)\n", ANSI_YELLOW, ANSI_RESET, ANSI_YELLOW, ANSI_RESET);
    printf("  %s--no-desktop%s               Launch direct without wine virtual desktop\n", ANSI_YELLOW, ANSI_RESET);
    printf("  %s--debug%s                    Enable full Wine debug messages (unsets WINEDEBUG=-all)\n\n", ANSI_YELLOW, ANSI_RESET);

    printf("%sADD OPTIONS (for '%s add'):%s\n", ANSI_BOLD, prog_name, ANSI_RESET);
    printf("  %s-r%s, %s--res <WxH>%s          Virtual desktop resolution (default: auto-detected)\n", ANSI_YELLOW, ANSI_RESET, ANSI_YELLOW, ANSI_RESET);
    printf("  %s-d%s, %s--desktop-name <str>%s Virtual desktop window title (default: game name)\n", ANSI_YELLOW, ANSI_RESET, ANSI_YELLOW, ANSI_RESET);
    printf("  %s-p%s, %s--prefix <path>%s      Custom WINEPREFIX (e.g. ~/.wine-nfs)\n", ANSI_YELLOW, ANSI_RESET, ANSI_YELLOW, ANSI_RESET);
    printf("  %s-w%s, %s--workdir <dir>%s      Custom working directory\n", ANSI_YELLOW, ANSI_RESET, ANSI_YELLOW, ANSI_RESET);
    printf("  %s-e%s, %s--env <KEY=VAL>%s      Custom environment variables\n", ANSI_YELLOW, ANSI_RESET, ANSI_YELLOW, ANSI_RESET);
    printf("  %s--no-desktop%s               Disable virtual desktop mode\n\n", ANSI_YELLOW, ANSI_RESET);

    printf("%sEXAMPLES:%s\n", ANSI_BOLD, ANSI_RESET);
    printf("  %s%s add%s                                    # Interactive setup wizard\n", ANSI_DIM, prog_name, ANSI_RESET);
    printf("  %s%s add nfsmw ~/Games/Nfsmw2005/speed.exe%s   # Quick add with default settings\n", ANSI_DIM, prog_name, ANSI_RESET);
    printf("  %s%s nfsmw%s                                  # Launch Need For Speed Most Wanted\n", ANSI_DIM, prog_name, ANSI_RESET);
    printf("  %s%s nfsmw --foreground%s                     # Launch in foreground to see logs\n", ANSI_DIM, prog_name, ANSI_RESET);
    printf("  %s%s logs nfsmw%s                             # View game output logs\n", ANSI_DIM, prog_name, ANSI_RESET);
    printf("  %s%s kill nfsmw%s                             # Terminate game\n\n", ANSI_DIM, prog_name, ANSI_RESET);
}

static int cmd_list(void) {
    GameConfig *games = NULL;
    size_t count = 0;
    if (config_load_all(&games, &count) != 0) {
        fprintf(stderr, "%sError reading configuration.%s\n", ANSI_RED, ANSI_RESET);
        return 1;
    }

    if (count == 0) {
        printf("%sNo games registered yet!%s\n", ANSI_YELLOW, ANSI_RESET);
        printf("Add your first game with: %swinerunner add%s\n\n", ANSI_CYAN, ANSI_RESET);
        return 0;
    }

    printf("\n%sREGISTERED GAMES (%zu)%s\n", ANSI_BOLD, count, ANSI_RESET);
    printf("%s%-14s %-16s %-12s %-12s %s%s\n",
           ANSI_DIM, "ALIAS", "STATUS", "RESOLUTION", "DESKTOP", "EXECUTABLE PATH", ANSI_RESET);
    printf("%s──────────────────────────────────────────────────────────────────────────────────%s\n", ANSI_DIM, ANSI_RESET);

    for (size_t i = 0; i < count; i++) {
        GameConfig *g = &games[i];
        pid_t pid = 0;
        bool running = runner_is_running(g, &pid);

        char status_str[64];
        if (running) {
            snprintf(status_str, sizeof(status_str), "%s● Running [%d]%s", ANSI_BRIGHT_GREEN, pid, ANSI_RESET);
        } else {
            snprintf(status_str, sizeof(status_str), "%s○ Stopped%s", ANSI_DIM, ANSI_RESET);
        }

        char desktop_str[128];
        if (g->virtual_desktop) {
            snprintf(desktop_str, sizeof(desktop_str), "%sYes%s (%s)", ANSI_GREEN, ANSI_RESET,
                     g->desktop_name[0] ? g->desktop_name : g->name);
        } else {
            snprintf(desktop_str, sizeof(desktop_str), "%sNo%s", ANSI_DIM, ANSI_RESET);
        }

        printf("%s%-14s%s %-26s %-12s %-22s %s%s%s\n",
               ANSI_BOLD ANSI_CYAN, g->name, ANSI_RESET,
               status_str,
               g->resolution[0] ? g->resolution : "default",
               desktop_str,
               ANSI_WHITE, g->exe_path, ANSI_RESET);
    }
    printf("%s──────────────────────────────────────────────────────────────────────────────────%s\n\n", ANSI_DIM, ANSI_RESET);

    config_free_list(games);
    return 0;
}

static int cmd_info(const char *name) {
    if (!name || *name == '\0') {
        fprintf(stderr, "%sError: Game alias required. Usage: winerunner info <game-name>%s\n", ANSI_RED, ANSI_RESET);
        return 1;
    }

    GameConfig g;
    if (config_find(name, &g) != 0) {
        fprintf(stderr, "%sError: Game '%s' not found. Run 'winerunner list' to see all games.%s\n",
                ANSI_RED, name, ANSI_RESET);
        return 1;
    }

    pid_t pid = 0;
    bool running = runner_is_running(&g, &pid);

    char log_file[PATH_MAX];
    snprintf(log_file, sizeof(log_file), "%s/%s.log", config_get_logs_dir(), g.name);

    printf("\n%sGAME PROFILE: %s%s%s%s\n", ANSI_BOLD, ANSI_CYAN, g.name, ANSI_RESET, ANSI_BOLD);
    printf("  %s•%s Status            : %s\n", ANSI_DIM, ANSI_RESET,
           running ? ANSI_BRIGHT_GREEN "● Running" ANSI_RESET : ANSI_DIM "○ Stopped" ANSI_RESET);
    if (running) {
        printf("  %s•%s PID               : %d\n", ANSI_DIM, ANSI_RESET, pid);
    }
    printf("  %s•%s Executable Path   : %s%s%s\n", ANSI_DIM, ANSI_RESET, ANSI_BOLD, g.exe_path, ANSI_RESET);
    printf("  %s•%s Working Directory : %s\n", ANSI_DIM, ANSI_RESET, g.workdir[0] ? g.workdir : "(derived from exe)");
    printf("  %s•%s Virtual Desktop   : %s\n", ANSI_DIM, ANSI_RESET, g.virtual_desktop ? "Enabled" : "Disabled");
    if (g.virtual_desktop) {
        printf("  %s•%s Desktop Name      : %s\n", ANSI_DIM, ANSI_RESET, g.desktop_name[0] ? g.desktop_name : g.name);
        printf("  %s•%s Resolution        : %s\n", ANSI_DIM, ANSI_RESET, g.resolution[0] ? g.resolution : "1920x1080");
    }
    printf("  %s•%s Wine Prefix       : %s\n", ANSI_DIM, ANSI_RESET, g.wineprefix[0] ? g.wineprefix : "(default: ~/.wine)");
    if (g.env_vars[0] != '\0') {
        printf("  %s•%s Environment Vars  : %s\n", ANSI_DIM, ANSI_RESET, g.env_vars);
    }
    if (g.extra_args[0] != '\0') {
        printf("  %s•%s Extra Arguments   : %s\n", ANSI_DIM, ANSI_RESET, g.extra_args);
    }
    printf("  %s•%s Log File          : %s\n\n", ANSI_DIM, ANSI_RESET, log_file);

    return 0;
}

static int cmd_remove(const char *name) {
    if (!name || *name == '\0') {
        fprintf(stderr, "%sError: Game alias required. Usage: winerunner remove <game-name>%s\n", ANSI_RED, ANSI_RESET);
        return 1;
    }

    GameConfig g;
    if (config_find(name, &g) != 0) {
        fprintf(stderr, "%sError: Game '%s' not found.%s\n", ANSI_RED, name, ANSI_RESET);
        return 1;
    }

    printf("Are you sure you want to remove '%s%s%s'? ", ANSI_BOLD ANSI_CYAN, name, ANSI_RESET);
    if (!prompt_yes_no("", true)) {
        printf("Cancelled.\n");
        return 0;
    }

    if (config_remove(name) == 0) {
        printf("%s✓ Successfully removed '%s' from winerunner.%s\n", ANSI_GREEN, name, ANSI_RESET);
        return 0;
    } else {
        fprintf(stderr, "%sFailed to remove '%s'.%s\n", ANSI_RED, name, ANSI_RESET);
        return 1;
    }
}

static int cmd_add(int argc, char **argv) {
    GameConfig g;
    memset(&g, 0, sizeof(GameConfig));
    g.virtual_desktop = true;

    char default_res[MAX_RES_LEN];
    detect_screen_resolution(default_res, sizeof(default_res));
    snprintf(g.resolution, sizeof(g.resolution), "%s", default_res);

    bool interactive = false;
    char arg_name[MAX_GAME_NAME_LEN] = {0};
    char arg_path[MAX_PATH_LEN] = {0};

    /* Check positional arguments */
    int non_flag_count = 0;
    for (int i = 0; i < argc; i++) {
        if (argv[i][0] == '-') {
            if (strcmp(argv[i], "--no-desktop") == 0) {
                g.virtual_desktop = false;
            } else if ((strcmp(argv[i], "-r") == 0 || strcmp(argv[i], "--res") == 0) && i + 1 < argc) {
                snprintf(g.resolution, sizeof(g.resolution), "%s", argv[++i]);
            } else if ((strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--desktop-name") == 0) && i + 1 < argc) {
                snprintf(g.desktop_name, sizeof(g.desktop_name), "%s", argv[++i]);
            } else if ((strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--prefix") == 0) && i + 1 < argc) {
                snprintf(g.wineprefix, sizeof(g.wineprefix), "%s", argv[++i]);
            } else if ((strcmp(argv[i], "-w") == 0 || strcmp(argv[i], "--workdir") == 0) && i + 1 < argc) {
                snprintf(g.workdir, sizeof(g.workdir), "%s", argv[++i]);
            } else if ((strcmp(argv[i], "-e") == 0 || strcmp(argv[i], "--env") == 0) && i + 1 < argc) {
                snprintf(g.env_vars, sizeof(g.env_vars), "%s", argv[++i]);
            } else if (strcmp(argv[i], "--args") == 0 && i + 1 < argc) {
                snprintf(g.extra_args, sizeof(g.extra_args), "%s", argv[++i]);
            }
        } else {
            if (non_flag_count == 0) {
                snprintf(arg_name, sizeof(arg_name), "%s", argv[i]);
            } else if (non_flag_count == 1) {
                snprintf(arg_path, sizeof(arg_path), "%s", argv[i]);
            }
            non_flag_count++;
        }
    }

    if (arg_name[0] == '\0' || arg_path[0] == '\0') {
        interactive = true;
    }

    if (interactive) {
        printf("\n%s🎮 Add a New Game to Winerunner%s\n", ANSI_BOLD ANSI_CYAN, ANSI_RESET);
        printf("%s──────────────────────────────────────────────────────────────────────────%s\n", ANSI_DIM, ANSI_RESET);

        /* 1. Game Name */
        prompt_string("Game alias (e.g. nfsmw, cod4)", arg_name[0] ? arg_name : NULL, g.name, sizeof(g.name));
        if (g.name[0] == '\0') {
            fprintf(stderr, "%sError: Game alias cannot be empty.%s\n", ANSI_RED, ANSI_RESET);
            return 1;
        }

        /* 2. Path to executable */
        char path_buf[MAX_PATH_LEN];
        prompt_string("Path to game executable (.exe)", arg_path[0] ? arg_path : NULL, path_buf, sizeof(path_buf));
        if (path_buf[0] == '\0') {
            fprintf(stderr, "%sError: Executable path cannot be empty.%s\n", ANSI_RED, ANSI_RESET);
            return 1;
        }

        char *expanded = expand_path(path_buf);
        if (expanded) {
            snprintf(g.exe_path, sizeof(g.exe_path), "%s", expanded);
            free(expanded);
        } else {
            snprintf(g.exe_path, sizeof(g.exe_path), "%s", path_buf);
        }

        if (!file_exists(g.exe_path)) {
            printf("%s⚠️  Warning: File '%s' does not exist right now.%s\n", ANSI_YELLOW, g.exe_path, ANSI_RESET);
            printf("Save anyway? ");
            if (!prompt_yes_no("", true)) {
                printf("Aborted.\n");
                return 1;
            }
        }

        /* Working directory default */
        char *dir = get_directory_of_file(g.exe_path);
        if (dir) {
            snprintf(g.workdir, sizeof(g.workdir), "%s", dir);
            free(dir);
        }

        /* Virtual desktop */
        g.virtual_desktop = prompt_yes_no("Enable Wine Virtual Desktop (explorer)?", true);

        if (g.virtual_desktop) {
            prompt_string("Virtual desktop resolution", default_res, g.resolution, sizeof(g.resolution));
            prompt_string("Desktop window title", g.name, g.desktop_name, sizeof(g.desktop_name));
        }

        /* Wine prefix */
        prompt_string("Custom WINEPREFIX (press Enter for default ~/.wine)", NULL, g.wineprefix, sizeof(g.wineprefix));

        /* Environment variables */
        prompt_string("Custom environment variables (e.g. DXVK_HUD=fps, optional)", NULL, g.env_vars, sizeof(g.env_vars));

    } else {
        /* Non-interactive CLI flags */
        snprintf(g.name, sizeof(g.name), "%s", arg_name);
        char *expanded = expand_path(arg_path);
        if (expanded) {
            snprintf(g.exe_path, sizeof(g.exe_path), "%s", expanded);
            free(expanded);
        } else {
            snprintf(g.exe_path, sizeof(g.exe_path), "%s", arg_path);
        }

        if (g.workdir[0] == '\0') {
            char *dir = get_directory_of_file(g.exe_path);
            if (dir) {
                snprintf(g.workdir, sizeof(g.workdir), "%s", dir);
                free(dir);
            }
        }

        if (g.desktop_name[0] == '\0') {
            snprintf(g.desktop_name, sizeof(g.desktop_name), "%s", g.name);
        }

        if (!file_exists(g.exe_path)) {
            fprintf(stderr, "%s⚠️  Warning: Executable '%s' was not found.%s\n",
                    ANSI_YELLOW, g.exe_path, ANSI_RESET);
        }
    }

    if (config_add_or_update(&g) == 0) {
        printf("\n%s✓ Successfully registered '%s'!%s\n", ANSI_BOLD ANSI_GREEN, g.name, ANSI_RESET);
        printf("  %s•%s Executable : %s\n", ANSI_DIM, ANSI_RESET, g.exe_path);
        printf("  %s•%s Resolution : %s\n", ANSI_DIM, ANSI_RESET, g.resolution);
        printf("  %s•%s Desktop    : %s\n", ANSI_DIM, ANSI_RESET, g.virtual_desktop ? "Enabled" : "Disabled");
        printf("\n%sYou can now launch it by typing:%s\n", ANSI_DIM, ANSI_RESET);
        printf("  %swinerunner %s%s\n\n", ANSI_BOLD ANSI_CYAN, g.name, ANSI_RESET);
        return 0;
    } else {
        fprintf(stderr, "%sError saving configuration.%s\n", ANSI_RED, ANSI_RESET);
        return 1;
    }
}

int main(int argc, char **argv) {
    if (argc < 2) {
        print_banner();
        cmd_list();
        printf("%sFor help and available commands:%s winerunner --help\n\n", ANSI_DIM, ANSI_RESET);
        return 0;
    }

    const char *cmd = argv[1];

    if (strcmp(cmd, "--help") == 0 || strcmp(cmd, "-h") == 0 || strcmp(cmd, "help") == 0) {
        print_help(argv[0]);
        return 0;
    }

    if (strcmp(cmd, "--version") == 0 || strcmp(cmd, "-v") == 0 || strcmp(cmd, "version") == 0) {
        printf("%s %s\n", WINERUNNER_NAME, WINERUNNER_VERSION);
        return 0;
    }

    if (strcmp(cmd, "list") == 0 || strcmp(cmd, "ls") == 0) {
        return cmd_list();
    }

    if (strcmp(cmd, "add") == 0) {
        return cmd_add(argc - 2, argv + 2);
    }

    if (strcmp(cmd, "remove") == 0 || strcmp(cmd, "rm") == 0 || strcmp(cmd, "del") == 0) {
        if (argc < 3) {
            fprintf(stderr, "%sError: Specify game name to remove. Usage: winerunner remove <game-name>%s\n",
                    ANSI_RED, ANSI_RESET);
            return 1;
        }
        return cmd_remove(argv[2]);
    }

    if (strcmp(cmd, "info") == 0) {
        if (argc < 3) {
            fprintf(stderr, "%sError: Specify game name. Usage: winerunner info <game-name>%s\n",
                    ANSI_RED, ANSI_RESET);
            return 1;
        }
        return cmd_info(argv[2]);
    }

    if (strcmp(cmd, "logs") == 0 || strcmp(cmd, "log") == 0) {
        if (argc < 3) {
            fprintf(stderr, "%sError: Specify game name. Usage: winerunner logs <game-name> [-n lines]%s\n",
                    ANSI_RED, ANSI_RESET);
            return 1;
        }
        int lines = 40;
        if (argc >= 5 && strcmp(argv[3], "-n") == 0) {
            lines = atoi(argv[4]);
        }
        GameConfig g;
        if (config_find(argv[2], &g) != 0) {
            fprintf(stderr, "%sError: Game '%s' not found.%s\n", ANSI_RED, argv[2], ANSI_RESET);
            return 1;
        }
        return runner_show_logs(&g, lines);
    }

    if (strcmp(cmd, "kill") == 0 || strcmp(cmd, "stop") == 0) {
        if (argc < 3) {
            fprintf(stderr, "%sError: Specify game name. Usage: winerunner kill <game-name>%s\n",
                    ANSI_RED, ANSI_RESET);
            return 1;
        }
        GameConfig g;
        if (config_find(argv[2], &g) != 0) {
            fprintf(stderr, "%sError: Game '%s' not found.%s\n", ANSI_RED, argv[2], ANSI_RESET);
            return 1;
        }
        return runner_kill(&g);
    }

    if (strcmp(cmd, "config") == 0) {
        printf("%sConfig file:%s %s\n", ANSI_BOLD, ANSI_RESET, config_get_file_path());
        printf("%sLogs dir:   %s %s\n", ANSI_BOLD, ANSI_RESET, config_get_logs_dir());
        return 0;
    }

    /* Otherwise, cmd is treated as a <game-name> to run! */
    GameConfig g;
    if (config_find(cmd, &g) == 0) {
        RunOptions opts;
        memset(&opts, 0, sizeof(RunOptions));

        for (int i = 2; i < argc; i++) {
            if (strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "--foreground") == 0) {
                opts.foreground = true;
            } else if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--detached") == 0) {
                opts.foreground = false;
            } else if (strcmp(argv[i], "--no-desktop") == 0) {
                opts.force_no_desktop = true;
            } else if (strcmp(argv[i], "--debug") == 0) {
                opts.debug_wine = true;
            } else if ((strcmp(argv[i], "-r") == 0 || strcmp(argv[i], "--res") == 0) && i + 1 < argc) {
                strncpy(opts.override_res, argv[++i], sizeof(opts.override_res) - 1);
            }
        }

        return runner_launch(&g, &opts);
    }

    /* Not found as game or command */
    fprintf(stderr, "%sError: Unknown command or game '%s'.%s\n", ANSI_RED, cmd, ANSI_RESET);
    printf("Type %swinerunner list%s to see registered games, or %swinerunner --help%s for usage.\n\n",
           ANSI_CYAN, ANSI_RESET, ANSI_CYAN, ANSI_RESET);
    return 1;
}
