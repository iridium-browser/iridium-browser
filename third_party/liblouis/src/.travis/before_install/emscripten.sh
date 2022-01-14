if [ -z "$TRAVIS_COMMIT" ]; then
    echo "[liblouis-js] not building in travis. Aborting..."
    exit 1
fi

export COMMIT_SHORT=$(echo $TRAVIS_COMMIT | cut -c1-6)

echo $TRAVIS_TAG | grep "^v[0-9][0-9]*\.[0-9][0-9]*\.[0-9][0-9]*$"

if [ $? -eq 1 ]; then
	echo "[liblouis-js] tag is not valid version string."
	export BUILD_VERSION="commit-${COMMIT_SHORT}"
	export IS_OFFICIAL_RELEASE=false
else
	# NOTE: tags cannot be revoked. Only automatically publish as release
	# candidate. A contributer should confirm the correctness of the build
	# and rerelease unaltered binaries without the -rc suffix.
	export BUILD_VERSION="${TRAVIS_TAG}-rc.1"
	export IS_OFFICIAL_RELEASE=true
fi

echo "[liblouis-js] Assigned this build the version number ${BUILD_VERSION}" &&

# --- obtain liblouis-js. Contains tests and js snippets appended to builds.
#     liblouis-js version should be incremented by hand, to keep the repositories
#     in sync.
git clone https://github.com/liblouis/liblouis-js.git &&
cd liblouis-js &&
git checkout 8a28e9380c591c58e4b411bb366c76cf686ac418 &&
cd .. &&
# --- obtain the latest version of liblouis/js-build
#     we publish/deploy to this repository. Contains package
#     descriptions (package.json and bower.json) and documentation
#     that must/should be part of packages in package managers.

# Note: we clone this repository to a location outside of the liblouis/liblouis
# git repository to avoid issues caused by nested git repositorys
git clone --depth 1 https://github.com/liblouis/js-build.git ../js-build &&

echo "[liblouis-js] obtaining docker image of build tools..." &&
docker pull dolp/liblouis-js-build-travis:1.37.3-64bit

