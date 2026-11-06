# kitten ssh only enables shell integration for the login user; enable it for sudo/su sessions too
if status is-interactive; and test "$TERM" = xterm-kitty; and not set -q KITTY_SHELL_INTEGRATION
    set -gx KITTY_SHELL_INTEGRATION enabled
end
