-----BEGIN PGP SIGNED MESSAGE-----
Hash: SHA512

Format: 3.0 (quilt)
Source: openldap
Binary: slapd, slapd-contrib, slapd-smbk5pwd, ldap-utils, libldap2, libldap-common, libldap-dev, libldap2-dev, slapi-dev
Architecture: any all
Version: 2.6.7+dfsg-1~exp1ubuntu8.2
Maintainer: Ubuntu Developers <ubuntu-devel-discuss@lists.ubuntu.com>
Uploaders: Steve Langasek <vorlon@debian.org>, Torsten Landschoff <torsten@debian.org>, Ryan Tandy <ryan@nardis.ca>, Sergio Durigan Junior <sergiodj@debian.org>
Homepage: https://www.openldap.org/
Standards-Version: 4.6.2
Vcs-Browser: https://salsa.debian.org/openldap-team/openldap
Vcs-Git: https://salsa.debian.org/openldap-team/openldap.git
Testsuite: autopkgtest
Testsuite-Triggers: heimdal-kdc, openssl, samba, schema2ldif, ssl-cert
Build-Depends: debhelper-compat (= 13), dh-apparmor, dpkg-dev (>= 1.17.14), groff-base, heimdal-multidev (>= 7.4.0.dfsg.1-1~) <!pkg.openldap.noslapd>, libargon2-dev <!pkg.openldap.noslapd>, libgnutls28-dev, libltdl-dev <!pkg.openldap.noslapd>, libperl-dev (>= 5.8.0) <!pkg.openldap.noslapd>, libsasl2-dev, libwrap0-dev <!pkg.openldap.noslapd>, nettle-dev <!pkg.openldap.noslapd>, openssl <!nocheck>, perl:any, pkg-config (>= 0.29), po-debconf, unixodbc-dev <!pkg.openldap.noslapd>, krb5-admin-server <!nocheck>, krb5-user <!nocheck>, krb5-kdc <!nocheck>, libsasl2-modules-gssapi-mit <!nocheck>, sasl2-bin <!nocheck>
Build-Conflicts: autoconf2.13, bind-dev, libbind-dev
Package-List:
 ldap-utils deb net optional arch=any
 libldap-common deb libs optional arch=all
 libldap-dev deb libdevel optional arch=any
 libldap2 deb libs optional arch=any
 libldap2-dev deb oldlibs optional arch=all
 slapd deb net optional arch=any profile=!pkg.openldap.noslapd
 slapd-contrib deb net optional arch=any profile=!pkg.openldap.noslapd
 slapd-smbk5pwd deb oldlibs optional arch=all profile=!pkg.openldap.noslapd
 slapi-dev deb libdevel optional arch=any profile=!pkg.openldap.noslapd
Checksums-Sha1:
 80f065b81f691416ea376e10d25ecab3b9e39fb1 3774648 openldap_2.6.7+dfsg.orig.tar.xz
 0ea7c88e6669487b106cf0b947b87a4fc905a10c 186792 openldap_2.6.7+dfsg-1~exp1ubuntu8.2.debian.tar.xz
Checksums-Sha256:
 ac9e26700ff8dd10181944d3e61014df5e3db601dd05655966be46757710d6df 3774648 openldap_2.6.7+dfsg.orig.tar.xz
 03ea2415780fcb27aef9b0ab63f170a698f8b7ba83fd4224580120b429606d5f 186792 openldap_2.6.7+dfsg-1~exp1ubuntu8.2.debian.tar.xz
Files:
 36c0542618d106ebef1c15e58b2d80ca 3774648 openldap_2.6.7+dfsg.orig.tar.xz
 338e05cbb15b4e8c5374c91b47f339dc 186792 openldap_2.6.7+dfsg-1~exp1ubuntu8.2.debian.tar.xz
Original-Maintainer: Debian OpenLDAP Maintainers <pkg-openldap-devel@lists.alioth.debian.org>

-----BEGIN PGP SIGNATURE-----

iQIzBAEBCgAdFiEEiYfY5F8zkxDOZhouZMRpvlnA3qIFAmeRzRcACgkQZMRpvlnA
3qJdJRAAuIeyTY6ByjycXDKS6G89F9p/ar/FDJwnwamcqOWECdDU02QBsbMdRHgb
VxF9CuPiOVRqhp/fBO9L8gLamG//kAFHPL2LQsN3GGH6rOxUSrvqO/fsEfDZyWFT
5x9RmXcIfXV6E59qt0Oe0gvJl/uEgBN8JCs8ftu6FCIGR+YqKdoU12BiZFGz9hTM
Oyb7fRmzHAaDhAiExGwirFbJbP8leHcWu/4zh5w33pARgdvxkjlQbYVXkEWAnsAV
fze4OJE7y76IjZqb58GR5juz4JrAAx+/J4j8YW2829S6jUHLFjyPe32NmifOntko
Mc6dEIQb8ap0p8cpHVE5dTBuLu/F7f6LGI/DoPUKKsZsOcH2Q8QscRaSHByVoN8b
s0VC52PfGxBYVbnStSUez+4xNXDLAfov0ujAGJAVRAL9rGzAoQqPUMCvNvhzxPly
em09kEiwSe+P6cHet60Ik5wiK63A3JTf0SFWHSTlVrZI2nIijHJNepVNiTlFB+Vv
5KZURIGNcDeHIoNCH2HE1TkMDjQDd9QG8zxQQ3O2OI5rAPztPXDKa9/t6v4JWIv8
LUpaI1UQ1v+J3ze6N5yZBnY6G9ZFnjEp0YqhXbsSCq/4hKX/RIs4iqDbK1GBvLKa
kqDnO7vUDGMJBFsQhR6VOJB+6J1YVGNI8AqIr1DjnoQKcaMg/Xk=
=5hPD
-----END PGP SIGNATURE-----
