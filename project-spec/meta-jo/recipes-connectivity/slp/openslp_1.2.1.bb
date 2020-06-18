DESCRIPTION = "OpenSLP 1.2.1"
LICENSE = "BSD"
LIC_FILES_CHKSUM = "file://${WORKDIR}/${PN}-${PV}/COPYING;md5=69974e24a9fae771ced9240f7eb0770f"
PR = "r1"

inherit autotools

PN = "openslp"

SRC_URI="http://sourceforge.net/projects/openslp/files/OpenSLP/1.2.1/openslp-1.2.1.tar.gz \
        file://slp.conf"

do_configure_append() {
    cp ${WORKDIR}/slp.conf ${S}/etc/
}

FILES_${PN} += "usr/*"

SRC_URI[md5sum] = "ff9999d1b44017281dd00ed2c4d32330"
SRC_URI[sha256sum] = "08c7ec1e76fdd66461b3784d52047f594405f31ba2791ab0c1ec7c97639f5fbd"
