-----BEGIN PGP SIGNED MESSAGE-----
Hash: SHA512

Format: 3.0 (quilt)
Source: glib2.0
Binary: libglib2.0-0t64, libglib2.0-tests, libglib2.0-udeb, libglib2.0-bin, libglib2.0-dev, libglib2.0-dev-bin, libglib2.0-data, libglib2.0-doc, libgirepository-2.0-0, libgirepository-2.0-dev, gir1.2-glib-2.0, gir1.2-glib-2.0-dev, gir1.2-girepository-3.0, gir1.2-girepository-3.0-dev
Architecture: any all
Version: 2.80.0-6ubuntu3.8
Maintainer: Ubuntu Developers <ubuntu-devel-discuss@lists.ubuntu.com>
Uploaders: Jeremy Bícha <jbicha@ubuntu.com>, Marco Trevisan (Treviño) <marco@ubuntu.com>, Simon McVittie <smcv@debian.org>
Homepage: https://wiki.gnome.org/Projects/GLib
Standards-Version: 4.6.2
Vcs-Browser: https://salsa.debian.org/gnome-team/glib/tree/ubuntu/noble
Vcs-Git: https://salsa.debian.org/gnome-team/glib.git -b ubuntu/noble
Testsuite: autopkgtest
Testsuite-Triggers: build-essential, dbus-daemon, dbus-x11, dconf-gsettings-backend, dpkg-repack, gnome-desktop-testing, gsettings-desktop-schemas, locales, locales-all, xauth, xvfb
Build-Depends: dbus-daemon <!nocheck> <!noinsttest>, debhelper-compat (= 13), dh-sequence-gnome, dh-sequence-python3, docbook-xml, docbook-xsl, dpkg-dev (>= 1.22.5), gettext, libdbus-1-dev <!nocheck> <!noinsttest>, libelf-dev, libffi-dev, libmount-dev [linux-any], libpcre2-dev, libselinux1-dev [linux-any], libxml2-utils, linux-libc-dev [linux-any], meson (>= 1.2.0), pkgconf, python3-docutils <!nodoc>, python3-packaging:native, python3:native, xsltproc, zlib1g-dev
Build-Depends-Arch: desktop-file-utils <!nocheck>, dh-sequence-gir <!nogir>, gobject-introspection (>= 1.78.1-18~) <!nogir>, locales <!nocheck> | locales-all <!nocheck>, python3-dbus <!nocheck>, python3-gi <!nocheck>, qemu-user <cross !nogir>, shared-mime-info <!nocheck>, tzdata <!nocheck>, xterm <!nocheck>
Build-Depends-Indep: gi-docgen <!nodoc>, gobject-introspection (>= 1.78.1-18~) <!nodoc>, qemu-user <cross !nodoc>
Package-List:
 gir1.2-girepository-3.0 deb introspection optional arch=any profile=!nogir
 gir1.2-girepository-3.0-dev deb libdevel optional arch=any profile=!nogir
 gir1.2-glib-2.0 deb introspection optional arch=any profile=!nogir
 gir1.2-glib-2.0-dev deb libdevel optional arch=any profile=!nogir
 libgirepository-2.0-0 deb libs optional arch=any
 libgirepository-2.0-dev deb libdevel optional arch=any
 libglib2.0-0t64 deb libs optional arch=any
 libglib2.0-bin deb misc optional arch=any
 libglib2.0-data deb libs optional arch=all
 libglib2.0-dev deb libdevel optional arch=any
 libglib2.0-dev-bin deb libdevel optional arch=any
 libglib2.0-doc deb doc optional arch=all profile=!nodoc
 libglib2.0-tests deb libs optional arch=any profile=!noinsttest,!nogir
 libglib2.0-udeb udeb debian-installer optional arch=any profile=!noudeb
Checksums-Sha1:
 f9c0c1f83778f34ccfa40563d21a3535fd0485a0 263364 glib2.0_2.80.0.orig-unicode-data.tar.xz
 3416bb2ce53f451afb288e948bb7c1570307e606 5510536 glib2.0_2.80.0.orig.tar.xz
 b555953854083320ed32270a0f4ff6750bf0c06e 166540 glib2.0_2.80.0-6ubuntu3.8.debian.tar.xz
Checksums-Sha256:
 38680f78a0ae6258826418cb5096c19ae3566ba8fee0a2112a0ec40056e58729 263364 glib2.0_2.80.0.orig-unicode-data.tar.xz
 8228a92f92a412160b139ae68b6345bd28f24434a7b5af150ebe21ff587a561d 5510536 glib2.0_2.80.0.orig.tar.xz
 3ee18682434b0213fbc7f5892527f748810a657aba2353d660c5eb30ed1656f0 166540 glib2.0_2.80.0-6ubuntu3.8.debian.tar.xz
Files:
 52f85a65b58be1c5f2b4d0d943cff489 263364 glib2.0_2.80.0.orig-unicode-data.tar.xz
 3a51e2803ecd22c2dadcd07d9475ebe3 5510536 glib2.0_2.80.0.orig.tar.xz
 59a1d2f46976856edcfe16b8c8e4d798 166540 glib2.0_2.80.0-6ubuntu3.8.debian.tar.xz
Original-Maintainer: Debian GNOME Maintainers <pkg-gnome-maintainers@lists.alioth.debian.org>

-----BEGIN PGP SIGNATURE-----

iQIzBAEBCgAdFiEEUMSg3c8x5FLOsZtRZWnYVadEvpMFAml6XSIACgkQZWnYVadE
vpNGGA//TvbPjyBUeKZW66WbKOSYfPl7v+I3mOv7E4UzLfYLqlXUAHropTKUnNcb
OrTqFID4OOj5zhDM/+3EuS00cnEKALN+xz0wLKzIGayKnj/i6ODRWG7pzR8+zswd
TiBS0FWCU9rrqHSEK95nNYrUEDzqg2IY5AvxRqXWU0ibXzfzgQG5f4fyuHpZrRGK
YVoD7hxmPo4ODyryVlFNcFudVy65/dAyevKjk3Dk1ziub5xkbKMi9jh4vHgZXLq9
+HbJTbD1JEvjczs1dnlpwmZ9EymYo4X9LtTz5WDEa6PvRDCXS5yr/42kxom4K/YF
Ga7MHESmboOghjihdkJgLBjIDvn4Lc7f5nTlWeQdikytZoZjBcqbrI+WkFcknzPm
lM9/N7GtqnDKA0ICNVMDUyAhBm+xEpBlltJg75oe4ykilehfZJETbFYLMhZTw8xZ
xm0wt4eFhap9MPhqbCeYsuYsstPFFhgY+qHzsATJ0xao5aBqJUOvQs4t3TJV9tHM
W+4FGCHAolxzecnHFW99Eqg6iLcQbaMNrqct0HMtxKLL+X5nrVKbNiOWQ8T/tmGE
BqXWOStscBMnBkkWnmEjA+gNwoRJHR2GSv21q5Vc0OENn6z8ZcrbXzAfgczdGMB7
i1eumTBmXtxoaA0Cbfxu7PgrA0EjSxVwSwoz6Nq+UFR2zRrJ7hQ=
=Icj4
-----END PGP SIGNATURE-----
