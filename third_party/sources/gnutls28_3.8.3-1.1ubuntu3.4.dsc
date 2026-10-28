-----BEGIN PGP SIGNED MESSAGE-----
Hash: SHA512

Format: 3.0 (quilt)
Source: gnutls28
Binary: libgnutls28-dev, libgnutls30t64, gnutls-bin, gnutls-doc, libgnutls-openssl27t64, libgnutls-dane0t64
Architecture: any all
Version: 3.8.3-1.1ubuntu3.4
Maintainer: Ubuntu Developers <ubuntu-devel-discuss@lists.ubuntu.com>
Uploaders:  Andreas Metzler <ametzler@debian.org>, Eric Dorland <eric@debian.org>, James Westby <jw+debian@jameswestby.net>, Simon Josefsson <simon@josefsson.org>,
Homepage: https://www.gnutls.org/
Standards-Version: 4.6.2
Vcs-Browser: https://salsa.debian.org/gnutls-team/gnutls
Vcs-Git: https://salsa.debian.org/gnutls-team/gnutls.git
Testsuite: autopkgtest
Testsuite-Triggers: build-essential, ca-certificates, datefudge, freebsd-net-tools, net-tools, openssl, softhsm2
Build-Depends: dpkg-dev (>= 1.22.5), bison, ca-certificates <!nocheck>, chrpath, datefudge <!nocheck>, debhelper-compat (= 13), freebsd-net-tools [kfreebsd-i386 kfreebsd-amd64] <!nocheck>, gperf, libcmocka-dev <!nocheck>, libidn2-dev, libp11-kit-dev, libssl-dev <!nocheck>, libtasn1-6-dev, libunbound-dev, libunistring-dev, net-tools [!kfreebsd-i386 !kfreebsd-amd64] <!nocheck>, nettle-dev (>= 3.6), openssl <!nocheck>, pkg-config, python3:any, softhsm2 <!nocheck>
Build-Depends-Indep: gtk-doc-tools, texinfo, texlive-latex-base, texlive-plain-generic
Build-Conflicts: libgnutls-dev
Package-List:
 gnutls-bin deb net optional arch=any
 gnutls-doc deb doc optional arch=all
 libgnutls-dane0t64 deb libs optional arch=any
 libgnutls-openssl27t64 deb libs optional arch=any
 libgnutls28-dev deb libdevel optional arch=any
 libgnutls30t64 deb libs optional arch=any
Checksums-Sha1:
 806156ac9563caab642d6274496b9cc5b2117612 6463720 gnutls28_3.8.3.orig.tar.xz
 e256237d54d45e4b6f46fe692651d6dea33f359c 854 gnutls28_3.8.3.orig.tar.xz.asc
 170eb1387eedca9f7e39e82bc0a3330be9f6e5b7 100104 gnutls28_3.8.3-1.1ubuntu3.4.debian.tar.xz
Checksums-Sha256:
 f74fc5954b27d4ec6dfbb11dea987888b5b124289a3703afcada0ee520f4173e 6463720 gnutls28_3.8.3.orig.tar.xz
 b2b90d225728890b0e2aa7c05e5f25f8ba1282821b46e72cd99f0c732b639cef 854 gnutls28_3.8.3.orig.tar.xz.asc
 2839cedf1673eaa7c4183dd89e1f33b7144707d67e50803c706253b6dfff1fae 100104 gnutls28_3.8.3-1.1ubuntu3.4.debian.tar.xz
Files:
 269966167fa5bf8bae5f7534bcc3c454 6463720 gnutls28_3.8.3.orig.tar.xz
 8da0da357d6762b98a491bf0a3fc6e1f 854 gnutls28_3.8.3.orig.tar.xz.asc
 fbc17231f0d9d7a57a2dc04afc798aac 100104 gnutls28_3.8.3-1.1ubuntu3.4.debian.tar.xz
Original-Maintainer: Debian GnuTLS Maintainers <pkg-gnutls-maint@lists.alioth.debian.org>

-----BEGIN PGP SIGNATURE-----

iQIzBAEBCgAdFiEEUMSg3c8x5FLOsZtRZWnYVadEvpMFAmhxPzoACgkQZWnYVadE
vpNjThAAiHxkYjGMjXLaV8AeH3niTRcyo3W1u+nx2KFJB2UMmcf3CHipHPD+r4DA
5D71bQ0VELuQf0g64vfMa+RsYHRbj9FVQDPLQ9uHDQqHUNcyjpbzLePzJd+JslKl
W+CE0v/zxl/5zLZIfgPucwvH4Xo90G7gp6otBGM4NQB6Pd36GQYvRTNgswIHfBAW
l5FKEhY3sI2VVouZ8HaO1s3eChfjNn8qcNzQQ5u13ukYmgGiUQPO5X53AWHPgUwM
ApqtASu21YY1DbdhamVOAr0ZUEA9/I04iBYsG1fljjM32eXn6Wom2FbblsHarWjj
ETBKonmqeKoCYixR0nCf+437ajhBv9BwSJdQhymsGFdLS6FppueASxv1IwjTqmd7
vuxG1bRx5yKZFgZNN5DGzuIGl16tNXZJIx2oQWprHI0Bg1EGVbS0KBhM2T8u8DZr
BV7xR6uv37f2+AfTo0q0anwUsZ/DRg4M508eFNC2IsLFVuVsSwUP6Mmie/CLD0Zv
liCUkbex/L2bBpku4Zso8hXDJBNClxQZKUqeAl6zLzY+S2LthXhWQQAmgU7L76gw
Ax54LrP5r/OK45KBoLiq2kFd23xK0fpBoCXUihv2ti9H+UUCeMZh1xdaeXWYjkFg
hixKxrO9n+YfLtB9EiddvlFXuMb16dsorcN41mpakgBhaxyEpSg=
=sTWp
-----END PGP SIGNATURE-----
