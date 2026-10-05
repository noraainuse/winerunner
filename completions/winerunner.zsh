#compdef winerunner wineplay

_winerunner() {
    local -a commands
    commands=(
        'add:Register a new game with interactive wizard or flags'
        'remove:Remove a registered game'
        'rm:Remove a registered game'
        'list:List all registered games and statuses'
        'ls:List all registered games'
        'info:Show detailed configuration profile'
        'logs:View recent log output for a game'
        'kill:Terminate running instances of a game'
        'stop:Terminate running instances of a game'
        'config:Show config file path and logs folder'
        'help:Show help message'
        'version:Show version information'
    )

    local conf_file="${XDG_CONFIG_HOME:-$HOME/.config}/winerunner/games.conf"
    local -a games
    if [[ -f "$conf_file" ]]; then
        games=(${(f)"$(grep -E '^\[[a-zA-Z0-9_-]+\]' "$conf_file" | tr -d '[]')"})
    fi

    local -a run_opts
    run_opts=(
        '-f[Run in foreground to stream logs]'
        '--foreground[Run in foreground to stream logs]'
        '-d[Run detached in background]'
        '--detached[Run detached in background]'
        '-r[Override virtual desktop resolution]:resolution:(1920x1080 2560x1440 3840x2160 1280x720)'
        '--res[Override virtual desktop resolution]:resolution:(1920x1080 2560x1440 3840x2160 1280x720)'
        '--no-desktop[Launch direct without virtual desktop explorer]'
        '--debug[Enable full Wine debug logging]'
    )

    if (( CURRENT == 2 )); then
        _describe -t commands 'winerunner commands' commands
        if (( ${#games} > 0 )); then
            _describe -t games 'registered games' games
        fi
        return
    fi

    local subcmd="${words[2]}"
    case "$subcmd" in
        remove|rm|del|info|logs|kill|stop)
            _describe -t games 'registered games' games
            ;;
        add)
            _files
            ;;
        *)
            if (( ${games[(Ie)$subcmd]} )); then
                _arguments -s $run_opts
            fi
            ;;
    esac
}

_winerunner "$@"
