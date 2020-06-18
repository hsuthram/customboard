#
# This file is the mbsonar recipe.
#

SUMMARY = "Simple mbsonar application"
SECTION = "PETALINUX/apps"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://src \
           file://common/ \
           file://LSVC/ \
	   file://Makefile \
		  "

TARGET_CC_ARCH += "${LDFLAGS}"

DEPENDS += "\
           openslp \
           "

S = "${WORKDIR}"

do_compile() {
	     oe_runmake
}

do_install() {
	     install -d ${D}${bindir}
	     install -m 0755 mbsonar ${D}${bindir}
}

#INSANE_SKIP_${PN} = "ldflags"

#FILES_${PN} = "/usr/bin/mbsonar"
