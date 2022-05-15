# Copyright 1999-2011 Gentoo Foundation
# Distributed under the terms of the GNU General Public License v2
# $Header: $

EAPI=2
CROS_WORKON_PROJECT="chromiumos/third_party/glmark2"
inherit toolchain-funcs cros-workon

DESCRIPTION="Opengl test suite"

LICENSE="GPL-3"
SLOT="0"
KEYWORDS="amd64 arm x86"
IUSE="glesv2 drm"

RDEPEND="media-libs/libpng
	media-libs/mesa
	x11-libs/libX11"
DEPEND="${RDEPEND}
	dev-util/pkgconfig"

src_configure() {
	: ${WAF_BINARY:="${S}/waf"}

	local myconf

	if use glesv2; then
		myconf+="--enable-glesv2"
	fi

	if use drm; then
		myconf+=" --enable-gl-drm"
		if use glesv2; then
			myconf+=" --enable-glesv2-drm"
		fi
	fi

	tc-export CC CXX AR RANLIB LD NM PKG_CONFIG

	# it does not know --libdir specification, dandy huh
	CCFLAGS="${CFLAGS}" LINKFLAGS="${LDFLAGS}" "${WAF_BINARY}" \
		--prefix=/usr \
		--enable-gl \
		${myconf} \
		configure || die "configure failed"
}

src_compile() {
	${WAF_BINARY} ${CCFLAGS}  || die
}

src_install() {
	${WAF_BINARY} install --destdir="${D}" || die
}
