-----BEGIN PGP SIGNED MESSAGE-----
Hash: SHA512

Format: 3.0 (quilt)
Source: openssl
Binary: openssl, libssl3t64, libcrypto3-udeb, libssl3-udeb, libssl-dev, libssl-doc
Architecture: any all
Version: 3.0.13-0ubuntu3.6
Maintainer: Ubuntu Developers <ubuntu-devel-discuss@lists.ubuntu.com>
Uploaders: Christoph Martin <christoph.martin@uni-mainz.de>, Kurt Roeckx <kurt@roeckx.be>, Sebastian Andrzej Siewior <sebastian@breakpoint.cc>
Homepage: https://www.openssl.org/
Standards-Version: 4.6.2
Vcs-Browser: https://salsa.debian.org/debian/openssl
Vcs-Git: https://salsa.debian.org/debian/openssl.git
Testsuite: autopkgtest
Testsuite-Triggers: perl
Build-Depends: dpkg-dev (>= 1.22.5), debhelper-compat (= 13)
Package-List:
 libcrypto3-udeb udeb debian-installer optional arch=any profile=!noudeb
 libssl-dev deb libdevel optional arch=any
 libssl-doc deb doc optional arch=all
 libssl3-udeb udeb debian-installer optional arch=any profile=!noudeb
 libssl3t64 deb libs optional arch=any
 openssl deb utils optional arch=any
Checksums-Sha1:
 18b985dcd3fc0bab54cc4bfc10fa9a80ce9e345d 15294843 openssl_3.0.13.orig.tar.gz
 4c8862e2be63a7f7312cc893b737d5f18070c319 169500 openssl_3.0.13-0ubuntu3.6.debian.tar.xz
Checksums-Sha256:
 88525753f79d3bec27d2fa7c66aa0b92b3aa9498dafd93d7cfa4b3780cdae313 15294843 openssl_3.0.13.orig.tar.gz
 90d26d623ef6e8fe23ab0829e80b863844572f269ad43d6a9d91da08103f90b5 169500 openssl_3.0.13-0ubuntu3.6.debian.tar.xz
Files:
 c15e53a62711002901d3515ac8b30b86 15294843 openssl_3.0.13.orig.tar.gz
 03b4056733fa8827f7ab3824010c34d4 169500 openssl_3.0.13-0ubuntu3.6.debian.tar.xz
Original-Maintainer: Debian OpenSSL Team <pkg-openssl-devel@alioth-lists.debian.net>

-----BEGIN PGP SIGNATURE-----

iQIzBAEBCgAdFiEEUMSg3c8x5FLOsZtRZWnYVadEvpMFAmjStkEACgkQZWnYVadE
vpOInQ/+N9Yb/xP5CCyM4wK8RuvVtFT5ejduE4eaA14+ktcqKwsqMvbbk4idYS76
RNvemRicEvL4MbyXYv7QMnDpvqukSOLpaa8me6nrRn8HbuvHC38wk6i5AoOpANRK
Lc+hhMNHbtMyFsROPAMOCyPa6VLMvq3876zmUP31Yx9ttQYb6KEncwPytvwz3YJ6
6oU23W+o+pRFsYMx4pytJu8saQZU5bLNiZfob8C6Flsj0qbm8+3pgg7YEDV2rRKd
BR1a9AXGTW6mhlqyNeNQuJw5JWuOUDb0hdz2uw03ps5/bqjPias2TV/uODKd14yH
CAEkBOpJDOGe7nD5Dpt+H5w/OOW8eT4ErzT8hL8IFIEtwjHYm3TA/qxLbsp7xkoh
eGOuhemDt25wEEDfdea+7cO6Giy3LeuHPmcVIRlciEaWPPh/j2Y487Wuc0icMsnU
Wz9Gilb5Ehs5mOK2ijky3txNWYbZgrDLiP2N0gAHPwzwzf6ap4S4Oi1hgynPCzO1
5/yGmkUnwX2bI9vzaKTm1yPTwAGVRwV7sHXUMJ1st+zypdNUFdHUW1H/d+psyNuV
8yIbgTvvEYZvypSdUrFaqJLs1hJe5oiA9fmV3NsLLg900tKRl8bp4psC/kYLinSX
pc0/ti3jYH88VSmH6Te3XcH6WuE3u3jID9LZ37os3YZcmjAzfKI=
=Eps2
-----END PGP SIGNATURE-----
