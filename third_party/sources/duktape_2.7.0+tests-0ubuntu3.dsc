-----BEGIN PGP SIGNED MESSAGE-----
Hash: SHA512

Format: 3.0 (quilt)
Source: duktape
Binary: duktape, duktape-dev, libduktape207
Architecture: any
Version: 2.7.0+tests-0ubuntu3
Maintainer: Ubuntu Developers <ubuntu-devel-discuss@lists.ubuntu.com>
Uploaders: Thorsten Alteholz <debian@alteholz.de>
Homepage: https://duktape.org
Standards-Version: 4.6.0
Vcs-Browser: https://salsa.debian.org/debian-iot-team/duktape
Vcs-Git: https://salsa.debian.org/debian-iot-team/duktape.git
Testsuite: autopkgtest
Testsuite-Triggers: @builddeps@, build-essential, nodejs, pkg-config
Build-Depends: debhelper-compat (= 13), dh-exec
Package-List:
 duktape deb interpreters optional arch=any
 duktape-dev deb libdevel optional arch=any
 libduktape207 deb libs optional arch=any
Checksums-Sha1:
 139dc16e0984b4d1b4ec485d693102b118be1a6a 14764880 duktape_2.7.0+tests.orig.tar.xz
 b443257fe02355e59f1b0c589a2930551c3e0599 9128 duktape_2.7.0+tests-0ubuntu3.debian.tar.xz
Checksums-Sha256:
 012fe77bffc8daf8f68c6932c1c926ce155c09aed9a3705fb4606c19bfd093fd 14764880 duktape_2.7.0+tests.orig.tar.xz
 8faa4eeba9079ad47392d07c55c19ab34e9de514a9c8ec2700f4b8a38aabd660 9128 duktape_2.7.0+tests-0ubuntu3.debian.tar.xz
Files:
 caaf1f4819d78e10e28c28cc9f0b48e3 14764880 duktape_2.7.0+tests.orig.tar.xz
 db8d7d9c37d87fb3561d0dc3d23fcebb 9128 duktape_2.7.0+tests-0ubuntu3.debian.tar.xz
Original-Maintainer: Debian IoT Maintainers <debian-iot-maintainers@lists.alioth.debian.org>

-----BEGIN PGP SIGNATURE-----

iQJHBAEBCgAxFiEET7WIqEwt3nmnTHeHb6RY3R2wP3EFAmYUE/gTHGp1bGlhbmtA
dWJ1bnR1LmNvbQAKCRBvpFjdHbA/cWLwEACL447xRT4WJIH5oE7HO16sialrpOe0
+Xgh4Yh40Z9M+qPnblOT6u+RdHa8dytWNNUx0Pt8+GHs9t8gjEo9SDtTJO3WgZ+L
mFRq0I86zs4m2ZNz8wM5VzfVWDTlIVj1bRO6nu43bnDKNwGhAmnk+lFTowGVhEGq
BFYCJblDSnBOJXvSkgfj63QgDUJ4gjB/rRAOYN0fReor3xQG7bCUQlccdnFcX6th
68hMaIwmiguLRgKi7u8VC1er6JBfhXO747CJ5rnF6vwF7HbvBbcj/GHD7YFVekOC
tNJnqXghqj0oAwUg3i0Cx7U23Mnx9waTBU93yCwO8Nx5pRt3iWN6P3Rk2JbCOqLT
AwQ4F/M/LVCCu+/125e8mU2NxJnd47FcZlp+hF51ixjBmv0ce+vgVFxUK8hXGaUY
rI5PvQznUUbfrlZEhXkxiHHW7zLNS+0I16t36nMWUDqnHQ1cn780cVF2u0TXQeIs
701sh9k9A4XXFEQhdbpdVLROzwCmK6TBWtyW7jYt/Es9/MT025kidKWsjg1J+spj
aSbS8vj6WmTDyug2fn+zKllzPVjMEheBkPXjuLqFMy2dYBNJYX1h/jA7fwum71yF
TpxUiOY0XRW3i6OMhTa/U22oMeZykC7Bc+SbC3NV7zdyRW5USkO99uAiTe0WjYxa
xkWbG1mGkaCx7Q==
=bQUP
-----END PGP SIGNATURE-----
