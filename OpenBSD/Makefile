# $OpenBSD$

MAINTAINER = 	Marc Espie <espie@openbsd.org>
CATEGORIES =	multimedia
COMMENT =	closed caption subtitles extractor
HOMEPAGE = 	http://ccextractor.sourceforge.net/
V =		0.56
DISTFILES =	ccextractor.${V:S/.//}-src.zip
MASTER_SITES =	${MASTER_SITE_SOURCEFORGE:=ccextractor/}
DISTNAME =	ccextractor-$V
WRKDIST =	${WRKDIR}/ccextractor.$V
WRKSRC =	${WRKDIST}/src

# GPL 2.0
PERMIT_DISTFILES_CDROM =	Yes
PERMIT_PACKAGE_CDROM =		Yes
PERMIT_DISTFILES_FTP =		Yes
PERMIT_PACKAGE_FTP =		Yes

DOCDIR = 	${PREFIX}/share/doc/ccextractor
do-configure:
	ln -s ${FILESDIR}/Makefile ${WRKSRC}/Makefile

do-install:
	${INSTALL_PROGRAM} ${WRKBUILD}/ccextractor ${PREFIX}/bin
	${INSTALL_DATA_DIR} ${DOCDIR}
	for i in ${WRKDIST}/docs/*.TXT; do \
		${INSTALL_DATA} $$i ${DOCDIR}; \
	done

WANTLIB +=	c m stdc++ 

.include <bsd.port.mk>
