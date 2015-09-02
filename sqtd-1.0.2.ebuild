# Copyright 1999-2013 Gentoo Foundation
# Distributed under the terms of the GNU General Public License v3
# $Header: $

EAPI="5"

inherit cmake-utils  git-2 

DESCRIPTION="Squd quotas traffic daemon"

HOMEPAGE="https://github.com/vershininvi/sqtd/wiki"

SRC_URI=""

EGIT_REPO_URI="https://github.com/vershininvi/sqtd"
EGIT_COMMIT=${P}

LICENSE="GPL-3"

SLOT="0"

KEYWORDS="~amd64 ~x86"

IUSE=""

COMMON_DEPEND=""

RDEPEND=""

DEPEND=""

S=${WORKDIR}/${P}

src_configure() {
    einfo "Configuring sqtd"
    cmake-utils_src_configure  || die  "Cmake: sqtd configuration failed"
}

src_install() {
   einfo "Installing sqtd"
   cmake-utils_src_install || die "Cmake: sqtd installation failed"
}

