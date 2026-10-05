# Bash completion for winerunner

_winerunner() {
    local cur prev words cword
    _init_completion || return

    local commands="add remove rm list ls info logs kill stop config help version"
    local run_opts="-f --foreground -d --detached -r --res --no-desktop --debug"

    # Extract registered game names from games.conf
    local conf_file="${XDG_CONFIG_HOME:-$HOME/.config}/winerunner/games.conf"
    local games=""
    if [[ -f "$conf_file" ]]; then
        games=$(grep -E '^\[' "$conf_file" | tr -d '[]' | tr '\n' ' ')
    fi

    if [[ $cword -eq 1 ]]; then
        COMPREPLY=( $(compgen -W "$commands $games" -- "$cur") )
        return 0
    fi

    case "$prev" in
        remove|rm|del|info|logs|kill|stop)
            COMPREPLY=( $(compgen -W "$games" -- "$cur") )
            return 0
            ;;
        add)
            # Default to filename completion
            _filedir
            return 0
            ;;
        -r|--res)
            COMPREPLY=( $(compgen -W "1920x1080 2560x1440 3840x2160 1280x720 1600x900" -- "$cur") )
            return 0
            ;;
    esac

    # If first argument is a game name, complete run options
    local first="${words[1]}"
    if [[ " $games " =~ " $first " ]]; then
        COMPREPLY=( $(compgen -W "$run_opts" -- "$cur") )
        return 0
    fi
}

complete -F _winerunner winerunner wineplay
