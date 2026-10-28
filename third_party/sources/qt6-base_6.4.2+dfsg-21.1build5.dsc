-----BEGIN PGP SIGNED MESSAGE-----
Hash: SHA512

Format: 3.0 (quilt)
Source: qt6-base
Binary: libqt6core6t64, libqt6concurrent6t64, libqt6dbus6t64, libqt6gui6t64, qt6-qpa-plugins, qt6-gtk-platformtheme, qt6-xdgdesktopportal-platformtheme, libqt6opengl6t64, libqt6openglwidgets6t64, libqt6network6t64, libqt6sql6t64, libqt6sql6-mysql, libqt6sql6-odbc, libqt6sql6-psql, libqt6sql6-sqlite, libqt6sql6-ibase, libqt6test6t64, libqt6widgets6t64, libqt6xml6t64, libqt6printsupport6t64, qt6-base-dev, qt6-base-private-dev, qt6-base-dev-tools, qt6-base-examples, qmake6-bin, qmake6, qt6-base-doc, qt6-base-doc-html, qt6-base-doc-dev
Architecture: any all
Version: 6.4.2+dfsg-21.1build5
Maintainer: Ubuntu Developers <ubuntu-devel-discuss@lists.ubuntu.com>
Uploaders: Patrick Franz <deltaone@debian.org>,
Homepage: https://www.qt.io/developers/
Standards-Version: 4.6.2
Vcs-Browser: https://salsa.debian.org/qt-kde-team/qt6/qt6-base
Vcs-Git: https://salsa.debian.org/qt-kde-team/qt6/qt6-base.git
Build-Depends: dpkg-dev (>= 1.22.5), cmake (>= 3.24~), debhelper-compat (= 13), default-libmysqlclient-dev, dh-exec, firebird-dev [linux-any], libatspi2.0-dev, libb2-dev, libcups2-dev, libdbus-1-dev, libdouble-conversion-dev, libdrm-dev [linux-any], libfontconfig-dev, libfreetype-dev, libgbm-dev [linux-any], libgl-dev, libgles-dev, libglib2.0-dev, libglu1-mesa-dev | libglu-dev, libgtk-3-dev, libharfbuzz-dev, libicu-dev, libinput-dev [linux-any], libjpeg-dev, libkrb5-dev, liblttng-ust-dev [linux-any], libmd4c-dev, libmd4c-html0-dev, libmtdev-dev [linux-any], libpcre2-dev, libpng-dev, libpq-dev, libproxy-dev, libsctp-dev [linux-any], libsqlite3-dev, libssl-dev, libsystemd-dev [linux-any], libts-dev, libudev-dev [linux-any], libvulkan-dev [linux-any], libwayland-dev [linux-any], libx11-dev, libx11-xcb-dev, libxcb-glx0-dev, libxcb-icccm4-dev, libxcb-image0-dev, libxcb-keysyms1-dev, libxcb-randr0-dev, libxcb-render-util0-dev, libxcb-render0-dev, libxcb-shape0-dev, libxcb-shm0-dev, libxcb-sync-dev, libxcb-util-dev, libxcb-xfixes0-dev, libxcb-xinerama0-dev, libxcb-xinput-dev, libxcb-xkb-dev, libxcb1-dev, libxext-dev, libxfixes-dev, libxi-dev, libxkbcommon-dev, libxkbcommon-x11-dev, libxrender-dev, libzstd-dev, ninja-build, pkg-config, pkg-kde-tools (>= 0.15.17~), qt6-base-dev:native <cross>, unixodbc-dev, zlib1g-dev
Build-Depends-Indep: qt6-base-dev (>= 6.4~) <!nodoc>, qt6-documentation-tools (>= 6.4~) <!nodoc>
Package-List:
 libqt6concurrent6t64 deb libs optional arch=any
 libqt6core6t64 deb libs optional arch=any
 libqt6dbus6t64 deb libs optional arch=any
 libqt6gui6t64 deb libs optional arch=any
 libqt6network6t64 deb libs optional arch=any
 libqt6opengl6t64 deb libs optional arch=any
 libqt6openglwidgets6t64 deb libs optional arch=any
 libqt6printsupport6t64 deb libs optional arch=any
 libqt6sql6-ibase deb libs optional arch=linux-any
 libqt6sql6-mysql deb libs optional arch=any
 libqt6sql6-odbc deb libs optional arch=any
 libqt6sql6-psql deb libs optional arch=any
 libqt6sql6-sqlite deb libs optional arch=any
 libqt6sql6t64 deb libs optional arch=any
 libqt6test6t64 deb libs optional arch=any
 libqt6widgets6t64 deb libs optional arch=any
 libqt6xml6t64 deb libs optional arch=any
 qmake6 deb devel optional arch=any
 qmake6-bin deb devel optional arch=any
 qt6-base-dev deb libdevel optional arch=any
 qt6-base-dev-tools deb devel optional arch=any
 qt6-base-doc deb doc optional arch=all
 qt6-base-doc-dev deb libdevel optional arch=all
 qt6-base-doc-html deb doc optional arch=all
 qt6-base-examples deb misc optional arch=any
 qt6-base-private-dev deb libdevel optional arch=any
 qt6-gtk-platformtheme deb libs optional arch=any
 qt6-qpa-plugins deb libs optional arch=any
 qt6-xdgdesktopportal-platformtheme deb libs optional arch=any
Checksums-Sha1:
 be3859a1f98f38ea4ebde1306b7c9cac7e4cb370 45943692 qt6-base_6.4.2+dfsg.orig.tar.xz
 d31f91326059f055cdbac8099de3ffc370d31818 191848 qt6-base_6.4.2+dfsg-21.1build5.debian.tar.xz
Checksums-Sha256:
 4f14b89a9463a540fd84a6718b8519a2297119b454ef547721015a9e9019ae9e 45943692 qt6-base_6.4.2+dfsg.orig.tar.xz
 80dcad2d79766b9dabe69b507f4622e7225be34d2c0ca6bdab77791f720f21fd 191848 qt6-base_6.4.2+dfsg-21.1build5.debian.tar.xz
Files:
 99724190bd6eb53b3f4b723e09e32304 45943692 qt6-base_6.4.2+dfsg.orig.tar.xz
 7bf467eb8b8f5b7c0969fef326e3fa5c 191848 qt6-base_6.4.2+dfsg-21.1build5.debian.tar.xz
Original-Maintainer: Debian Qt/KDE Maintainers <debian-qt-kde@lists.debian.org>

-----BEGIN PGP SIGNATURE-----

iQJOBAEBCgA4FiEErEg/aN5yj0PyIC/KVo0w8yGyEz0FAmYJCGcaHHN0ZXZlLmxh
bmdhc2VrQHVidW50dS5jb20ACgkQVo0w8yGyEz0ZKA//b2/qhjVz1aipcd0c9Uv4
xGbVAKaDeza33wN3LXRdjyq1zkxMaiWYykdriEUe83BLWCdK2ebLUYYeKuNE9S8a
3ViRtULc0XbHHqzviK/aTnMhymlCwobdgplZzXAdQy6Q7bUc5NJTrMbcWUYN30/E
/pU7PtU9OVvmuLJTE+xnSjgad8fLYy6ZwA1UvK4x8PuyrzMb65yG8E+PcCeRX0tp
yYpWjqRYLtK7m9qotck/wOEqMf7TuyLx4157bEHN0rI1HOJUV84fWtMZhk/sRIbr
jsPafPX+4Yn/NKUlm59xLYpzY6bwpRa8PlMz3HmWutpUL99xoKBwtVG5VDBU6Rd8
NCCAjWGqrPbPROHwLIoa1FIGawnQEb8L3XAmJZl34Bqd1l2ZBWw7LcTanZDr/Nc8
pnpaZaEdNa88nYKcc0re3KkWNHsjpsQXaRMFf4yLr8pKg8FXJuW5RP4AJf91qlyn
KmuQvIZeR09N3MMLTUIS9dEnMsVyTOPlfVfDqaJ7hr/wiHfIFE9szjG4e+9X/nVp
vTsrsvfgRRynmsJmQ1m6mRow8jKWaYBiON+pUhbDbMhPYEKVmu1yqOPh/LwAwb0T
fODHRHAc0T4HtL7W304yV3mVY99JkeUTVCYYttFJHRAn+g+rlCbKuxmP+tml1ssb
4nAk/YR1eocTJzomfeokr/E=
=0pxs
-----END PGP SIGNATURE-----
