CATWM
=====

     /\___/\
    ( o   o )  Made by cat...
    (  =^=  )
    (        )            ... for cat!
    (         )
    (          ))))))________________

Summary
-------

catwm is a very simple and lightweight tiling window manager.

Status
------

For the moment, the wm is not functionnal.

Modes
-----

It allow the "normal" mode of tiling window managers:

    --------------
    |        |___|
    |        |___|
    | Master |___|
    |        |___|
    |        |___|
    --------------

or

    --------------
    |            |
    |   Master   |
    |            |
    |____________|
    |     |      |
    --------------

or fullscreen mode

Compilation
-----------

Need Xlib librairies, then just type:
gcc -o catwm catwm.c -lX11 -Wall
