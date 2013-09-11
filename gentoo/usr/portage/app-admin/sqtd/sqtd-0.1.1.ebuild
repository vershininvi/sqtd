# Copyright 1999-2013 Gentoo Foundation
# Distributed under the terms of the GNU General Public License v3
# $Header: /var/cvsroot/gentoo-x86/net-analyzer/zabbix/zabbix-2.0.5.ebuild,v 1.3 2013/02/22 02:45:53 mattm Exp $

EAPI="2"

inherit flag-o-matic depend.php autotools  user

DESCRIPTION="sqtd  is s SQUID traffic quotation daemon"
HOMEPAGE="http://www.i_have_not_home.com/"
MY_P=${P/_/}
SRC_URI="http://prdownloads.sourceforge.net/sqtd/${MY_P}.tar.bz2"
LICENSE="GPL-3"
SLOT="0"

KEYWORDS="~amd64 ~x86"

IUSE=""

COMMON_DEPEND=""

RDEPEND=""

DEPEND=""

S=${WORKDIR}/${MY_P}

src_prepare() {
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
		sqtd \
		sqtc
	dodoc README INSTALL NEWS ChangeLog \
		${S}/doc/sqtd.conf.example 
	doman  	${S}/man/sqtd.ru.1 \
	        ${S}/man/sqtc.ru.1
	
}
