-----BEGIN PGP SIGNED MESSAGE-----
Hash: SHA512

Format: 3.0 (quilt)
Source: krb5
Binary: krb5-user, krb5-kdc, krb5-kdc-ldap, krb5-admin-server, krb5-kpropd, krb5-multidev, libkrb5-dev, libkrb5-dbg, krb5-pkinit, krb5-otp, krb5-k5tls, krb5-doc, libkrb5-3, libgssapi-krb5-2, libgssrpc4t64, libkadm5srv-mit12, libkadm5clnt-mit12, libk5crypto3, libkdb5-10t64, libkrb5support0, libkrad0, krb5-gss-samples, krb5-locales, libkrad-dev
Architecture: any all
Version: 1.20.1-6ubuntu2.6
Maintainer: Ubuntu Developers <ubuntu-devel-discuss@lists.ubuntu.com>
Uploaders: Russ Allbery <rra@debian.org>, Benjamin Kaduk <kaduk@mit.edu>
Homepage: https://web.mit.edu/kerberos/
Standards-Version: 4.6.2
Vcs-Browser: https://salsa.debian.org/debian/krb5
Vcs-Git: https://salsa.debian.org/debian/krb5
Testsuite: autopkgtest
Testsuite-Triggers: ldap-utils, libsasl2-modules-gssapi-mit, slapd
Build-Depends: dpkg-dev (>= 1.22.5), byacc | bison, comerr-dev, debhelper-compat (= 13), docbook-to-man, libkeyutils-dev [linux-any], libldap2-dev <!stage1>, libsasl2-dev <!stage1>, libssl-dev, libverto-dev (>= 0.2.4), pkg-config, ss-dev
Build-Depends-Arch: dpkg-dev (>= 1.22.5), keyutils [linux-any] <!nocheck>, libcmocka-dev <!nocheck>, python3-kdcproxy <!nocheck>, python3-pyrad <!nocheck>
Build-Depends-Indep: doxygen, doxygen-latex, python3, python3-cheetah, python3-lxml, python3-sphinx, tex-gyre
Package-List:
 krb5-admin-server deb net optional arch=any
 krb5-doc deb doc optional arch=all
 krb5-gss-samples deb net optional arch=any
 krb5-k5tls deb net optional arch=any
 krb5-kdc deb net optional arch=any
 krb5-kdc-ldap deb net optional arch=any profile=!stage1
 krb5-kpropd deb net optional arch=any
 krb5-locales deb localization optional arch=all
 krb5-multidev deb libdevel optional arch=any
 krb5-otp deb net optional arch=any
 krb5-pkinit deb net optional arch=any
 krb5-user deb net optional arch=any
 libgssapi-krb5-2 deb libs optional arch=any
 libgssrpc4t64 deb libs optional arch=any
 libk5crypto3 deb libs optional arch=any
 libkadm5clnt-mit12 deb libs optional arch=any
 libkadm5srv-mit12 deb libs optional arch=any
 libkdb5-10t64 deb libs optional arch=any
 libkrad-dev deb libdevel optional arch=any
 libkrad0 deb libs optional arch=any
 libkrb5-3 deb libs optional arch=any
 libkrb5-dbg deb debug optional arch=any
 libkrb5-dev deb libdevel optional arch=any
 libkrb5support0 deb libs optional arch=any
Checksums-Sha1:
 06278439a6cd5a2aa861d8e877451b794487534b 8661660 krb5_1.20.1.orig.tar.gz
 1cd01998135e3db3c4401b84459fb19ab8baabaf 833 krb5_1.20.1.orig.tar.gz.asc
 32d116bfdb25ac63f885e413eb6c84a08568703c 122284 krb5_1.20.1-6ubuntu2.6.debian.tar.xz
Checksums-Sha256:
 704aed49b19eb5a7178b34b2873620ec299db08752d6a8574f95d41879ab8851 8661660 krb5_1.20.1.orig.tar.gz
 2afeec5dbc586cc40b7975645e02b4c41c4d719dd02213e828c72d8239d55666 833 krb5_1.20.1.orig.tar.gz.asc
 58e9a5265de110bcebfaf1f88efc384673508302abed9ec189e2f8f9053abe70 122284 krb5_1.20.1-6ubuntu2.6.debian.tar.xz
Files:
 73f5780e7b587ccd8b8cfc10c965a686 8661660 krb5_1.20.1.orig.tar.gz
 46551f0a032aa02dccac3789a344e028 833 krb5_1.20.1.orig.tar.gz.asc
 b8b8a41526194e0f8a1c4f44134a5093 122284 krb5_1.20.1-6ubuntu2.6.debian.tar.xz
Original-Maintainer: Sam Hartman <hartmans@debian.org>

-----BEGIN PGP SIGNATURE-----

iQJTBAEBCgA9FiEELOLXZEFYQHcSWEHiyfW2m9Ldu6sFAmgsrTsfHGhsaWIua29y
emh5bnNreXlAY2Fub25pY2FsLmNvbQAKCRDJ9bab0t27q74kD/9NR1V4G40nd5up
PsjQ0BbzO20P2eNrMIXbxP7BCzuDFHvNp2t5t14yoM6MgdCmkiiRnZXhS34lQEii
QkHBs3jsDvQeXECVBPXwrIE5EjtCu8jzj3wv28pCe/vWgKNiG+4+hXo9HOYEgZwo
qjBAhvlF2pw1r8NUlUTi+YfwibhCWM3DcP94woNxQn/uKTuwnqbzhIFAOPLc4VZ8
/kwhfmrZOlLdVZhBrO7vsfxLdiP2bUIdJURtFOuXO2PDNmjrzbDHTE78KlzahgYQ
OEY1lLiLponiwMd9EoX3u/p+Aw0FNkxiLgxFCIcKj2zEXcH4hLJg9F6qfR5jWLz3
p5gDrL6X/wFLjSKLd7D82TZmVlegg2rJelNmZ1p3B8xPsvHdaH2jGwwyECO6mVe8
m8Wczsb+Jc1RLx3IZr902vpLDTTTQ0Qz+RjDTPAcz279ss/X9Sjtoynza0tq2ijj
bQTJQf7HVLWIJ144tMOPYVY2UID1Jo3VJbOKGpFEFQAZC7AZIVl7cRzMWzD4CUu2
eslfhzAxCgEzHaWTi2sJ2n+UB5nkGqWXO00fEU7D48pwh6v01IJUmC3kDG6Qtu7P
aKPO5QMUOv8jVElOoVkWBGS3zbeNT+kUYHD//ddpfJQeH+LX7+bEGwE3dZ3Y/40e
m+uNPpSWGtzBxwSDLluOlkObQwejHQ==
=E1kD
-----END PGP SIGNATURE-----
