-----BEGIN PGP SIGNED MESSAGE-----
Hash: SHA512

Format: 3.0 (quilt)
Source: nghttp2
Binary: libnghttp2-dev, libnghttp2-doc, libnghttp2-14, nghttp2-client, nghttp2-proxy, nghttp2-server, nghttp2
Architecture: any all
Version: 1.59.0-1ubuntu0.2
Maintainer: Ubuntu Developers <ubuntu-devel-discuss@lists.ubuntu.com>
Uploaders: Ondřej Surý <ondrej@debian.org>
Homepage: https://nghttp2.org/
Standards-Version: 4.6.2
Vcs-Browser: https://salsa.debian.org/debian/nghttp2
Vcs-Git: https://salsa.debian.org/debian/nghttp2.git
Testsuite: autopkgtest
Testsuite-Triggers: nginx
Build-Depends: debhelper (>= 13), debhelper-compat (= 13), libc-ares-dev, libcunit1-dev <!nocheck>, libev-dev, libjansson-dev, libjemalloc-dev [!hurd-i386], libssl-dev, libsystemd-dev, libxml2-dev, pkg-config, zlib1g-dev
Build-Depends-Indep: python3-sphinx, python3-sphinx-rtd-theme
Package-List:
 libnghttp2-14 deb libs optional arch=any
 libnghttp2-dev deb libdevel optional arch=any
 libnghttp2-doc deb doc optional arch=all
 nghttp2 deb httpd optional arch=all
 nghttp2-client deb httpd optional arch=any
 nghttp2-proxy deb httpd optional arch=any
 nghttp2-server deb httpd optional arch=any
Checksums-Sha1:
 ea1a278e07b4697f502b66a353ad8171657e6013 1055492 nghttp2_1.59.0.orig.tar.gz
 4086df7476c2a8e42bcb599f220a62ffe8c6d58a 14204 nghttp2_1.59.0-1ubuntu0.2.debian.tar.xz
Checksums-Sha256:
 0dd5c980f1262ff5f03676fd99f46f250b66c842f7d864fa1ca9f8453e5f8868 1055492 nghttp2_1.59.0.orig.tar.gz
 20392558c0dbdfa6d60efe56e6c3c958bffe1d52bc7569c7ac1be43f729e38ca 14204 nghttp2_1.59.0-1ubuntu0.2.debian.tar.xz
Files:
 7768dc83ead00b5833214204c2acfd5e 1055492 nghttp2_1.59.0.orig.tar.gz
 345702de74350cfc1bf1656ca99bef5f 14204 nghttp2_1.59.0-1ubuntu0.2.debian.tar.xz
Original-Maintainer: Tomasz Buchert <tomasz@debian.org>

-----BEGIN PGP SIGNATURE-----

iQIzBAEBCgAdFiEEVovyKmYzfL/Jprm3LIPbyOm9DjcFAmb+IZIACgkQLIPbyOm9
Djfyyg/+KCd9eIlc84kRM4BkUA1GkGFk2ZIwyYvTxMIigcKFZv8sYDfwmp/nJs+Q
wED95qNae3X8cNXFrfYaUnOWljpVNRuQ2o0Vwb98YB+YDl9a0CXkmbLJyFY3TkLh
F7eXS2wgK3o/i7mgwgaIqTF+BONm0fIgSwVMPbEiSFSVBD+gNrX2P3ry6yh+Fx/8
VzVid2mmYiWVjPWgB+ioXuxn5Fauw3oRPI1NF8HyiT8zb3ru5zIW9Lrm1rQ3I9Fw
H+MqKSXxcCtcuEejISMjpoahtnKTyBxmaQKjBVnVEfcaTE8fl/DkMdpD+r+x2D7O
hDckIewGs1i/SK7WWUu7517S7MLIG3Qtu3y4BeVLRkC4ckZnHmHRWVr26hNXCI8U
xkpCjWRTUWibORxXKf5masp8GkXiPPo6nDLufP0vi+GqpgqrJiOtR/n32qRxuXwT
e3PxGyqHSnjRBxWFhs22oLbr49HXz5pm2sz6coAZNDf0Ljd1IjwRw1dy4zSMQOjv
JNQbIVIrxA7/S9h9SSwgKndvKD0AM3RPl5EzHGifJMc1AsRZjLYA3Ids9Z04gwo+
Su0v7cOWE9y507iqSXcXkv1uxcNuS5Mp4mu3JV9Ei2bAtDoTkbnwdx7moJAenkpI
aXVqsxhljlWSEy1Hprn55aCh17B/VQich8qpbCVGXHbPpxE+KHM=
=adSu
-----END PGP SIGNATURE-----
