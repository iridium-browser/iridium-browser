if [ "$TRAVIS_PULL_REQUEST" != "false" -o "$TRAVIS_BRANCH" != "master" ]; then
	echo "[liblouis-js] Not publishing. Is pull request or non-master branch."
	exit 0
fi

if [ -z "$BUILD_VERSION" ]; then
	echo "[liblouis-js] no build version specified. Not publishing."
	exit 0
fi

echo "[liblouis-js] publishing builds to development channel..."

git config user.name "Travis CI" &&
git config user.email "liblouis@users.noreply.github.com" &&

# --- decrypt and enable ssh key that allows us to push to the
#     liblouis/js-build repository.

openssl aes-256-cbc -K $encrypted_cf3facfb36cf_key -iv $encrypted_cf3facfb36cf_iv -in ./.travis/secrets/deploy_key.enc -out deploy_key -d &&
chmod 600 deploy_key &&
eval `ssh-agent -s` &&
ssh-add deploy_key &&

# --- push commit and tag to repository. (This will also automatically
#     publish the package in the bower registry as the bower registry
#     just fetches tags and builds from the dev channel.)

cd ../js-build &&
git add --all &&

if [ -z `git diff --cached --exit-code` ]; then
	echo "[liblouis-js] Build is identical to previous build. Omitting commit, only adding tag."
else
	git commit -m "Automatic build of version ${BUILD_VERSION}" &&
	git push git@github.com:liblouis/js-build.git master

	if [ $? != 0 ]; then
		echo "[liblouis-js] Failed to commit. Aborting."
		exit 1
	fi
fi

git tag -a ${BUILD_VERSION} -m "automatic build for version ${BUILD_VERSION}" &&
git push git@github.com:liblouis/js-build.git $BUILD_VERSION

echo "[liblouis-js] publishing builds to release channel..."

if [ "$IS_OFFICIAL_RELEASE" != true ]; then
	echo "[liblouis-js] Is not an official release. Not publishing to package managers."
	exit 0
fi

# --- push in npm registry
# TODO
