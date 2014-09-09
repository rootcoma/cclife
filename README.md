cclife
===

Game of Life in C with Curses

# About
![life](http://i.imgur.com/8oyHr0y.gif)

Game is stored as a bitmap. Bits can be randomly generated anytime ('r') or set from the pause mode ('p').

# Requirements
- curses.h
- gcc (-std=gnu99)

# Usage
Compile and run
```
make && ./cclife
```

(optional)Rules are passed in Golly/RLE format
```
#Life rules
./cclife B3/S23

#HighLife rules
./cclife B36/S23
```

# Commands
###Global:
- q - quit
- p - toggle pause
- r - randomize
- c - clear/reset

###Paused:
- a - add bit at cursor
- d - remove bit at cursor
- hjkl - move cursor
- arrow keys - move cursor

###Running:
- n - decrease speed
- m - increase speed
