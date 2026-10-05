cmake_minimum_required(VERSION 3.16)
project(winerunner VERSION 1.0.0 LANGUAGES C)

# C Standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

# Compiler Warnings and Optimization
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -O2)
endif()

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Source files
set(SOURCES
    src/main.c
    src/config.c
    src/runner.c
    src/utils.c
)

# Executable target
add_executable(winerunner ${SOURCES})

# Installation configuration
include(GNUInstallDirs)

install(TARGETS winerunner
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Install shell completions
install(FILES completions/winerunner.bash
    DESTINATION ${CMAKE_INSTALL_DATADIR}/bash-completion/completions
    RENAME winerunner
    OPTIONAL
)

install(FILES completions/winerunner.zsh
    DESTINATION ${CMAKE_INSTALL_DATADIR}/zsh/site-functions
    RENAME _winerunner
    OPTIONAL
)

# Custom target: install-user (installs into ~/.local/bin without needing sudo)
add_custom_target(install-user
    COMMAND ${CMAKE_COMMAND} -E make_directory $ENV{HOME}/.local/bin
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:winerunner> $ENV{HOME}/.local/bin/winerunner
    COMMAND ${CMAKE_COMMAND} -E create_symlink $ENV{HOME}/.local/bin/winerunner $ENV{HOME}/.local/bin/wineplay
    COMMENT "Installing winerunner and wineplay symlink to ~/.local/bin"
    DEPENDS winerunner
)
