CATWM
=====

     /\___/\
    ( o   o )  Made by cat...
    (  =^=  )
    (        )            ... for cat!
    (         )
    (          ))))))________________ Cute And Tiny Window Manager

Summary
-------

catwm is a very simple and lightweight tiling window manager.
I will try to stay under 1000 SLOC.

Status
------

 * 24.02.10 -> First release, v0.1. In this release 0.1, the wm is almost functional

Modes
-----

It allow the "normal" modes of tiling window managers:

    --------------
    |        |___|
    |        |___|
    | Master |___|
    |        |___|
    |        |___|
    --------------

and fullscreen mode

There is no horizontal stack because I never use it. But if someone is interested in, it's very easy to add.

Installation
------------

Need Xlib, then:
    $ vim config.h
    $ make
    # make install
    $ make clean

Bugs
----
 * Application doesn't really close. The window is correctly unmapped, but the application is still running :(

Todo
----
 * Cleanup some parts of the code => Almost done
 * Add multiple view ("desktop")
 * Correct the close bug

If you have some particular request, just send me an e-mail, and I will see for it!

