cclife
===

Game of Life in C with Curses

# About
Game is stored as a bitmap. Bits can be randomly generated anytime ('r') or set from the pause mode ('p').

# Requirements
- curses.h
- sys/time.h (linux)

# Commands
###Global:
q - quit
p - toggle pause
r - randomize
###Pause:
a - add bit at cursor
d - remove bit at cursor
hjkl - move cursor
arrow keys - move cursor
###Running:
n - decrease speed
m - increase speed

