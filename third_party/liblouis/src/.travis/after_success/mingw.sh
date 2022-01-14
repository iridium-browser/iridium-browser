if [ "$TRAVIS_PULL_REQUEST" != "false" -o "$TRAVIS_BRANCH" != "master" ]; then
    echo "[mingw] Not publishing. Is pull request or non-master branch."
    exit 0
fi

echo "[mingw] Zipping up build..."

INSTALL_DIR=win$ARCH-install
COMMIT=$( git rev-parse --short=7 HEAD )
ZIP=liblouis-win$ARCH-$COMMIT.zip

make install && \
cd $INSTALL_DIR && \
zip -r $ZIP * &&  \
cd .. &&  \
mv $INSTALL_DIR/$ZIP .

if [ $? != 0 ]; then
    echo "[mingw] Failed to zip up build"
    exit 1
fi

RELEASE_ID=8031256
GITHUB_USER="bertfrees"

echo "[mingw] First deleting previous build"

# Using "curl -u" because authenticated requests get a higher rate limit
assets=$(curl -u "$GITHUB_USER:$GITHUB_TOKEN" \
              "https://api.github.com/repos/liblouis/liblouis/releases/$RELEASE_ID/assets" \
              2>/dev/null)
if echo "$assets" | jq -e '.message' >/dev/null 2>/dev/null; then
    echo "$assets" | jq -r '.message' >&2
    exit 1
else
    assets=$(echo "$assets" | jq -r '.[] | select(.name | match("^liblouis-win'$ARCH'-.+\\.zip$")) | .url') || exit 1
    echo "$assets" \
    | while read u; do
        if ! message=$(curl -u "$GITHUB_USER:$GITHUB_TOKEN" -X DELETE "$u" 2>/dev/null); then
            echo "[mingw] Failed to delete asset $u"
            exit 1
        elif [ -n "$message" ]; then
            echo "$message" | jq -r '.message' >&2
            echo "[mingw] Failed to delete asset $u"
            exit 1
        fi
    done
fi

echo "[mingw] Uploading builds to Github release..."

if ! curl -u "$GITHUB_USER:$GITHUB_TOKEN" \
     -H "Content-type: application/zip" \
     -X POST \
     "https://uploads.github.com/repos/liblouis/liblouis/releases/$RELEASE_ID/assets?name=$ZIP" \
     --data-binary @$ZIP \
     2>/dev/null \
    | jq -e '.url'
then
    echo "[mingw] Failed to upload asset"
    exit 1
fi

echo "[mingw] Editing release description..."

DESCRIPTION="Latest build: $COMMIT"
if ! curl -u "$GITHUB_USER:$GITHUB_TOKEN" \
     -H "Accept: application/json" \
     -H "Content-type: application/json" \
     -X PATCH \
     "https://api.github.com/repos/liblouis/liblouis/releases/$RELEASE_ID" \
     -d "{\"tag_name\": \"snapshot\", \
          \"body\":     \"$DESCRIPTION\"}" \
     2>/dev/null \
    | jq -e '.url'
then
    echo "[mingw] Failed to edit release description"
    exit 1
fi
