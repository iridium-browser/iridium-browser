cargo clean
rm -rf cov/rustc-semver/
rm -rf cov/*

RUSTFLAGS="-Zinstrument-coverage" \
LLVM_PROFILE_FILE="$(pwd)/cov/rustc-semver%m.profraw" \
    cargo +nightly test

llvm-profdata merge -sparse cov/rustc-semver*.profraw -o cov/rustc-semver.profdata

case $1 in
    "--json")
        llvm-cov export \
            --instr-profile=cov/rustc-semver.profdata \
            --summary-only \
            --format=text \
            $(find target/debug/deps -executable -type f) | python3 -m json.tool > cov.json
        ;;
    "--html")
        cargo install rustfilt
        llvm-cov show \
            --instr-profile=cov/rustc-semver.profdata \
            --Xdemangler=rustfilt \
            --show-line-counts-or-regions \
            --output-dir=cov/rustc-semver \
            --format=html \
            $(find target/debug/deps -executable -type f)
        ;;
    *)
        echo "Use with either --html or --json"
        exit 1
        ;;
esac

