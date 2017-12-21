
Debian
====================
This directory contains files used to package xc3d/xc3-qt
for Debian-based Linux systems. If you compile xc3d/xc3-qt yourself, there are some useful files here.

## xcurrency: URI support ##


xc3-qt.desktop  (Gnome / Open Desktop)
To install:

	sudo desktop-file-install xc3-qt.desktop
	sudo update-desktop-database

If you build yourself, you will either need to modify the paths in
the .desktop file or copy or symlink your blocknetdxqt binary to `/usr/bin`
and the `../../share/pixmaps/blocknetdx128.png` to `/usr/share/pixmaps`

xc3-qt.protocol (KDE)

