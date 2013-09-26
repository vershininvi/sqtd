# Copyright 1999-2013 Gentoo Foundation
# Distributed under the terms of the GNU General Public License v3
# $Header: $

EAPI="2"

inherit autotools git-2 

DESCRIPTION="Squd quotas traffic daemon"

HOMEPAGE="http://code.google.com/p/sqtd/"

SRC_URI=""

EGIT_REPO_URI="https://code.google.com/p/sqtd/"
EGIT_COMMIT=${P}

LICENSE="GPL-3"

SLOT="0"

KEYWORDS="~amd64 ~x86"

IUSE=""

COMMON_DEPEND=""

RDEPEND=""

DEPEND=""

S=${WORKDIR}/${P}

src_prepare() {
   echo ${S}
   eautoreconf
}

src_configure() {
    
    econf || die "econf failed"
}

src_install() {
	dodir \
               /etc/sqtd \
	       /var/lib  \
               /var/lib/sqtd
	insinto /etc/sqtd
	doins \
	       ${S}/doc/sqtd.conf.example 
	doinitd \
		${S}/etc/init.d/sqtd
        doconfd \
		${S}/etc/conf.d/sqtd
	dosbin \
		${S}/src/sqtd/sqtd \
		${S}/src/sqtc/sqtc
	domo \
		${S}/po/ru.gmo 
	dodoc README INSTALL NEWS ChangeLog \
		${S}/doc/sqtd.conf.example 
	doman  	${S}/man/sqtd.ru.1 \
	        ${S}/man/sqtc.ru.1
}
