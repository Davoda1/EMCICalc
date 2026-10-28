-----BEGIN PGP SIGNED MESSAGE-----
Hash: SHA512

Format: 3.0 (quilt)
Source: curl
Binary: curl, libcurl4t64, libcurl3t64-gnutls, libcurl4-openssl-dev, libcurl4-gnutls-dev, libcurl4-doc
Architecture: any all
Version: 8.5.0-2ubuntu10.13
Maintainer: Ubuntu Developers <ubuntu-devel-discuss@lists.ubuntu.com>
Uploaders: Carlos Henrique Lima Melara <charlesmelara@riseup.net>, Samuel Henrique <samueloph@debian.org>, Sergio Durigan Junior <sergiodj@debian.org>
Homepage: https://curl.se/
Standards-Version: 4.6.2
Vcs-Browser: https://salsa.debian.org/debian/curl
Vcs-Git: https://salsa.debian.org/debian/curl.git
Testsuite: autopkgtest
Testsuite-Triggers: @builddeps@, gcc, libc-dev, libcurl-dev, libldap-dev, pkgconf, slapd
Build-Depends: dpkg-dev (>= 1.22.5), debhelper-compat (= 13), autoconf, automake, ca-certificates, dh-exec, groff-base, libbrotli-dev, libgnutls28-dev, libidn2-dev, libkrb5-dev, libldap2-dev, libnghttp2-dev, libpsl-dev, librtmp-dev, libssh-dev, libssh2-1-dev, libssl-dev, libtool, libzstd-dev, locales-all <!nocheck>, openssh-server <!nocheck>, python3:native <!nocheck>, python3-impacket [!i386] <!nocheck>, gnutls-bin [amd64 arm64 armel armhf i386 mips64el mipsel s390x powerpc ppc64 riscv64] <!nocheck>, quilt, stunnel4 [!i386] <!nocheck>, zlib1g-dev
Build-Conflicts: autoconf2.13, automake1.4
Package-List:
 curl deb web optional arch=any
 libcurl3t64-gnutls deb libs optional arch=any
 libcurl4-doc deb doc optional arch=all
 libcurl4-gnutls-dev deb libdevel optional arch=any
 libcurl4-openssl-dev deb libdevel optional arch=any
 libcurl4t64 deb libs optional arch=any
Checksums-Sha1:
 d7877b39b6efda66e9ac5f9c28b8b28d8c0b7b7e 4372979 curl_8.5.0.orig.tar.gz
 46da770b703dd8a70d98aac68c76ae0118bdc2b5 89820 curl_8.5.0-2ubuntu10.13.debian.tar.xz
Checksums-Sha256:
 05fc17ff25b793a437a0906e0484b82172a9f4de02be5ed447e0cab8c3475add 4372979 curl_8.5.0.orig.tar.gz
 d11ea8d8546146b4367410b04945db4462832d262bb04ba81c97d145e86e9ce7 89820 curl_8.5.0-2ubuntu10.13.debian.tar.xz
Files:
 0bc69288b20ae165ff4b7d6d7bbe70d2 4372979 curl_8.5.0.orig.tar.gz
 4a0ba1baea7e66971103b1d3f9ac9ff9 89820 curl_8.5.0-2ubuntu10.13.debian.tar.xz
Original-Maintainer: Debian Curl Maintainers <team+curl@tracker.debian.org>

-----BEGIN PGP SIGNATURE-----

iQJPBAEBCgA5FiEEuEg4xBTSeUWY0Nx77tH0LvjAvecFAmqHa5obHGt5bGUua2Vy
bmlja0BjYW5vbmljYWwuY29tAAoJEO7R9C74wL3nmkIQAJ5n74BsOFIZ851GEl2P
ymPeE61nrstOMciUbcn4CVlEjmivs8gkdw/k/sqJPAMgn8nZdgPCUKVWZwhInOWk
yE/ZvDZvtAszNyXDbFqMq8e8YEhr1IC0NNkF/NjiWqGoLSAGA4Ju4q2AYMjzUDHQ
bACeEs8NlTGpstfuAxFmw4sIpDGqHLS81UyEIpzIX2WCuzf9uBp4tT3nIP+Lj0Jb
SfrVVlH/GNfN6qyVINj5ACw4Fqi7mhTwJdylQsZp5XztjHqty+9f7kSY2fgdaXsF
MqmKzMCqpjT0zwwUf1VOYUQ1teJMip8Cl9gkUpBHHq2bZuWKSmvxWopgLPLI5/0z
xRXx/zReKTsCuWOWSngwC3O9rvE/9UZ2x3N5oNKUDCGpNA+hdudt7MwJ6aZI0nrB
K7wq0UIadw0GlNzb97+DU0LGNHAKCuW+9XgxlsA/8tPJf4z0D1F3VYrB0EO/ggOf
fR2vhr26RI7cTu/uMaAImTQWGkjSyWA+gWDJl7pCdLRPWf3fwWH59OR/eJ8ncXCH
pw1aCKfjHBj42lgasNl6y9UEvcZSnjwYcBezKyY9GmLU/Xc3OpLxwKZXEe98B6IC
xzQeDRVAoNfQLu5bajxEvNXg7JkRXRPEhotG09nUDrDjSaB1APzzIroD+uhbfOj8
Cgb3yV6Gv7jgnzQr/y6DNZJq
=mv5V
-----END PGP SIGNATURE-----
