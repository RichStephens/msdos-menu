# msdos-menu

A small Open Watcom C launcher for programs running under MS-DOS. It scans the
current directory for up to 256 `.EXE` files, excluding itself.
Use Up/Down to select a program, PgUp/PgDn to change pages, Enter to run it,
and Esc to clear the screen and exit to DOS. The menu uses a centered title,
line-drawn frame, and reverse-video selection bar, and adapts to the current
text screen width and height. When the list spans multiple pages, arrows at
the right edge of the list frame indicate whether more pages are available,
and a `PAGE X OF Y` indicator shows the current position. Up/Down moves within
the current page and stops at its ends; PgUp/PgDn changes pages. Moving the
highlight does not redraw the rest of the screen. Before launching a program,
it shows `Running <PROGRAM>...` for one second, clears the screen immediately
before starting the program so its output begins on a clean display, and returns
directly to the menu when the selected program exits, without a status message.
The build limits the executable's maximum allocation to its minimum
requirement, so DOS does not reserve otherwise free memory for the menu while
a test runs.

## Build prerequisites

- GNU Make
- Open Watcom 2.x, with `wcc` and `wlink` available on `PATH`
- Python 3
- `mtools` (`mformat` and `mcopy`) to build the optional test disk

Build from this directory with the normal Make targets:

```sh
make
make clean
```

`make` builds `dosmenu.exe`; `make clean` removes the executable and object
file. To choose another executable basename, set `MENU_PROGRAM`, for example
`make MENU_PROGRAM=testmenu` builds `testmenu.exe`. For a reproducible
cross-platform toolchain, run `defoogi make` or
`defoogi make clean` from this directory. `defoogi` is an optional external
build wrapper; it is not called by the Makefile.

## Test disk

Run `make test` to create `testmenu-test.img`, a 360 KB DOS disk image with the
menu and 24 small demo programs. The default `make` builds only `dosmenu.exe`;
it does not build the test disk. Each demo displays its name and waits for a
key before returning to the menu, so the list spans multiple pages and
launch/return behavior can be exercised. `make testdisk` is an alias for
`make test`. Use `defoogi make test` to build it with the cross-platform
toolchain. `DEMO_NAMES` can be overridden to change the demo program list. The
image is not deployed automatically.

The displayed title defaults to `Program Menu` and can be changed at build
time, for example with `make MENU_TITLE="My Programs"`. The FujiNet testing
build uses `testmenu.exe` and sets the title to `FujiNet Test Programs`.

The FujiNet test build clones this repository into its local cache, sets the
program name to `testmenu`, sets the title to `FujiNet Test Programs`, and adds
`testmenu.exe` to the MS-DOS test image.
