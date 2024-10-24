# inputerceptor

## Keyboard and mouse interceptor for protection against pets and extra convenience

The main purpose of this utility is to lock your keyboard and mouse
when a cat jumps onto your table and/or starts walking on the keyboard, playing with the mouse, etc.

Also allows you to e.g. clean your keyboard safely without locking your system and having to enter the password,
or to let your kids bash away at your laptop without messing up your system.

This is NOT A SECURE LOCK, but a convenience one, use normal/other locking methods for security.

With some source code tweaking, you can add e.g.
key and/or button remapping,
keyboard layout switching with CapsLock,
mouse movement to scroll conversion for trackballs,
etc. Feel free to PR the features you think might be useful to others!)

## How to lock all input

Press LeftCtrl+Esc or RightCtrl+F12.

Having any other buttons or keys pressed at the same time will not prevent locking.

Keys that were pressed before locking will generate release events when released,
but no other events will be generated until input is unlocked.

## How to unlock input

When locked, follow these steps:

1. Release all keys and mouse buttons
1. Press and hold LeftCtrl+RightCtrl+Esc+F12
1. Release the keys

If you press any other keys or buttons while unlocking, you have to start over.

## Rationale for the above lock/unlock method

The locking combinations are just 2-key combos each, easy to trigger reliably with one hand.

The locking combinations are usually not used for anything else.
Ctrl+Esc opens the Start menu by default, but hardly anyone uses that combo any more
since the dedicated Windows key appeared on most keyboards a long time ago.
Ctrl+F12 is sometimes used, and using RightCtrl for it might be more convenient for some,
but this is quite a corner case and you can still use LeftCtrl+F12.

Having one locking combination on each end of the keyboard and ignoring other pressed keys
allows you to lock the keyboard when the cat is already on it,
unless it somehow manages to cover the whole keyboard left-to-right.

The chosen unlock method requires you to press 4 corner keys of the keyboard together,
without pressing any other keys around or between them even for a moment.
Easy enough for a sentient human, yet nigh-impossible for a cat. :D

If that scheme is not convenient for you for whatever reason, be that some app or a missing key,
you can always change it to your liking.

## Changing key bindings

Unfortunately, for now you have to edit the source code and rebuild the program.

## Building

The program should build just fine like any other MS Visual Studio project.
The "Debug" configuration is built as a console app, allowing you to see the debug output
and to close it with Ctrl-C or by closing the console window.

## License

This program is distributed under the MIT license:

> Copyright (c) 2024 Evgeniya Zhabotinskaya
> 
> Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
> 
> The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
> 
> THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
