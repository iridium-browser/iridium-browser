# apply clang-format and check for uncommitted changes
./autogen.sh && \
./configure && \
make format-sources && \
git diff-index --exit-code HEAD -- . ':!.travis'
