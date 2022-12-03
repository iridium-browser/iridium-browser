#!/bin/bash
set -eo pipefail

# Copyright 2018-2019 by
# Armin Hasitzka.
#
# This file is part of the FreeType project, and may only be used, modified,
# and distributed under the terms of the FreeType project license,
# LICENSE.TXT.  By continuing to use, modify, or distribute this file you
# indicate that you have read the license and understand and accept it
# fully.

dir="${PWD}"
cd "$( dirname "$( readlink -f "${0}" )" )" # go to `/fuzzing/scripts'

# ----------------------------------------------------------------------------
# collect parameters:

opt_help="0"   # 0|1
opt_rebuild="0" # 0|1

while [[ "${#}" -gt "0" ]]; do
    case "${1}" in
        --help)
            opt_help="1"
            shift
            ;;
        --rebuild)
            opt_rebuild="1"
            shift
            ;;
        *) # show usage when invalid parameters are used:
            opt_help="1"
            shift
            ;;
    esac
done

# ----------------------------------------------------------------------------
# usage:

if [[ "${opt_help}" == "1" ]]; then
    cat <<EOF

usage: ${0} [OPTIONS]

This script listens to a few environmental variables.  It is not mandatory to
set any of these variables but if they are set, they will be used.

  CFLAGS    Additional C compiler flags.
  CXXFLAGS  Additional C++ compiler flags.
  LDFLAGS   Additional linker flags.

OPTIONS:

  --rebuild  Rebuild the last build.  Nothing will be reset, nothing will be
             changed.  Using this option calls 'make' in every module without
             flushing or resetting anything.  Useful for debugging.

  --help  Print usage information.

EOF

    exit 66
fi

# ----------------------------------------------------------------------------
# rebuild shortcut:

if [[ "${opt_rebuild}" == "1" ]]; then
    # Each project must be listed after any project it depends on.
    bash build/zlib.sh       --no-init
    bash build/bzip2.sh      --no-init
    bash build/libarchive.sh --no-init
    bash build/brotli.sh     --no-init
    bash build/libpng.sh     --no-init
    bash build/freetype.sh   --no-init
    bash build/libcxx.sh     --no-init
    #bash build/glog.sh       --no-init
    bash build/targets.sh    --no-init
    exit
fi

# ----------------------------------------------------------------------------
# settings:

ansi_reset="\e[0m"
ansi_bold="\e[1m"
ansi_underline="\e[4m"
ansi_red="\e[31m"
ansi_yellow="\e[33m"

build_type=      # d|f
build_san=       # a|m|t|u|n
build_ubsan=     # y|n
build_coverage=  # y|n
build_debugging= # 0|1|2|3|n
build_ft_trace=  # y|n
build_ccache=    # y|n

cflags="${CFLAGS}"
cxxflags="${CXXFLAGS} -std=c++11"
ldflags="${LDFLAGS}"

# Base name of the driver:
driver_name="driver"

# ----------------------------------------------------------------------------
# helpers:

# Print the new line character (\n).
function print_nl()
{
    printf "\n"
}

# Print a question.
#
# $1: Question string.
function print_q()
{
    printf "\n${ansi_bold}%s${ansi_reset}\n" "${1}"
}

# Print some information.
#
# $1: Name of an option.
# $2: Description of the option.
#
# .. or ...
#
# $1: General information.
function print_info()
{
    if [[ "$#" == "1" ]]; then
        printf "  %s\n" "${1}"
    elif [[ "$#" == "2" ]]; then
        printf "  ${ansi_yellow}%s${ansi_reset}: %s\n" "${1}" "${2}"
    else
        exit 66
    fi
}

# Print a url.
#
# $1: URL.
function print_url()
{
    printf "  ${ansi_underline}%s${ansi_reset}\n" "${1}"
}

# Print (+ verify) the selection of an option.
#
# $1: The chosen option.
# ${2n + 2, n >= 0}: The nth option.
# ${2n + 3, n >= 0}: The nth description of an option.
#
# Example: $ print_sel ${selection} "y" "yes" "n" "no" ...
function print_sel()
{
    i=2
    while [[ i -le "$#" ]]; do
        if [[ "${1}" == "${!i}" ]]; then
            i=$(( i + 1 ))
            printf "\n  selection: ${ansi_yellow}%s${ansi_reset}\n" "${!i}"
            return
        fi
        i=$(( i + 2 ))
    done
    printf "\n  ${ansi_red}${ansi_bold}invalid selection: %s${ansi_reset}\n\n" "${1}"
    exit 66
}

# Print (+ verify) the slection of an option that can either be "y" (yes) or
# "n" (no).
#
# $1: The chosen option.
function print_sel_yes_no()
{
    print_sel "${1}" "y" "yes" "n" "no"
}

# Ask the user and print the (accepted + valid) result.
#
# $1: a list of options like "a|b|c"
# $2 (optional): a default option like "a"
function ask_user()
{
    options="${1}"
    if [[ "$#" == "2" ]]; then
        options="${options}, default: ${2}"
    fi
    while : ; do
        read -p "(${options}) > " answer
        answer=$(echo "${answer}" | tr '[:upper:]' '[:lower:]')
        if [[ "${answer}" == "" && "$#" == "2" ]]; then
            echo "${2}"
            break
        fi
        if [[ "${1}" == *"|${answer}|"* ||
              "${1}" == *"|${answer}"   ||
              "${1}" ==   "${answer}|"* ]]; then
            echo "${answer}"
            break
        fi
    done
}

# ----------------------------------------------------------------------------
# interaction:

printf "${ansi_yellow}"
printf " ____ ____  ____ ____ ______    __ ___  ____\n"
printf "|  __)  _ \|  __)  __)_   __)  / /  _ \|  __)\n"
printf "| |__| |_) ) |__| |__  | | \ \/ /| |_) ) |__\n"
printf "|  __)    /|  __)  __) | |  \  / |  __/|  __)\n"
printf "| |  | |\ \| |__| |__  | |  / /  | |   | |__\n"
printf "|_|  |_| \_\____)____) |_| /_/   |_|   |____)\n\n"
printf "${ansi_reset}"
printf "               Custom Build\n"

clang_version_regex='.*clang version ([0-9]+)\.[0-9]+.*'

if [[ "${CC}" =~ .*"clang".* ]]; then
    cc="${CC}"
else
    cc="clang"
fi
print_info "cc" "${cc}"
cc_version_output=$(${cc} --version)
if [[ "${cc_version_output}" =~ ${clang_version_regex} ]]; then
    cc_version="${BASH_REMATCH[1]}"
    print_info "cc version" "${cc_version}"
else
    print_info "cc version not detected"
fi

if [[ "${CXX}" =~ .*"clang".* ]]; then
    cxx="${CXX}"
else
    cxx="clang++"
fi
print_info "cxx" "${cxx}"
cxx_version_output=$(${cxx} --version)
if [[ "${cxx_version_output}" =~ ${clang_version_regex} ]]; then
    cxx_version="${BASH_REMATCH[1]}"
    print_info "cxx version" "${cxx_version}"
    if [ "${cxx_version}" -lt "10" ]; then
        print_info "cxx version too old. Need clang 10 or newer."
        exit 66
    fi
else
    print_info "cxx version not detected"
fi

print_q    "Build the driver or the fuzzer?"
print_info "driver" "run selected samples or failed instances"
print_info "fuzzer" "fuzz a corpus of samples with libFuzzer"
print_nl

build_type=$( ask_user "d|f" )
print_sel "${build_type}" "d" "driver" "f" "fuzzer"

if [[ "${build_type}" == "f" ]]; then
    cflags="  ${cflags}   -fsanitize=fuzzer-no-link"
    cxxflags="${cxxflags} -fsanitize=fuzzer-no-link"

    export CMAKE_FUZZING_ENGINE="-fsanitize=fuzzer"
fi

llvm_sanitizer=""

# Address, Memory, and Thread Sanitizers are incompatibile.
# Also, UndefinedBehavior should only be used by itself or with Address.
# See clang/lib/Driver/SanitizerArgs.cpp#IncompatibleGroups
print_q   "Sanitizer: Address, Memory, Thread, UndefinedBehavior, or None?"
print_url "https://clang.llvm.org/docs/AddressSanitizer.html"
print_url "https://clang.llvm.org/docs/MemorySanitizer.html"
print_url "https://clang.llvm.org/docs/ThreadSanitizer.html"
print_url "https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html"
print_nl

build_san=$( ask_user "a|m|t|u|n" "a" )
print_sel "${build_san}"           \
  "a" "AddressSanitizer"           \
  "m" "MemorySanitizer"            \
  "t" "ThreadSanitizer"            \
  "u" "UndefinedBehaviorSanitizer" \
  "n" "None"

if [[ "${build_san}" == "a" ]]; then
    cflags="  ${cflags}   -fsanitize=address -fsanitize-address-use-after-scope"
    cxxflags="${cxxflags} -fsanitize=address -fsanitize-address-use-after-scope"
    ldflags=" ${ldflags}  -fsanitize=address"
    driver_name="${driver_name}-asan"
    llvm_sanitizer="address"

    print_q   "Add the UndefinedBehaviorSanitizer?"
    print_url "https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html"
    print_nl
    build_ubsan=$( ask_user "y|n" "y" )
    print_sel_yes_no "${build_ubsan}"

elif [[ "${build_san}" == "m" ]]; then
    cflags="  ${cflags}   -fsanitize=memory -fsanitize-memory-track-origins"
    cxxflags="${cxxflags} -fsanitize=memory -fsanitize-memory-track-origins"
    ldflags=" ${ldflags}  -fsanitize=memory"
    driver_name="${driver_name}-msan"
    llvm_sanitizer="memory"
elif [[ "${build_san}" == "t" ]]; then
    cflags="  ${cflags}   -fsanitize=thread"
    cxxflags="${cxxflags} -fsanitize=thread"
    ldflags=" ${ldflags}  -fsanitize=thread"
    driver_name="${driver_name}-tsan"
    llvm_sanitizer="thread"
elif [[ "${build_san}" == "u" ]]; then
    build_ubsan="y"
fi

if [[ "${build_ubsan}" == "y" ]]; then
    cflags="  ${cflags}   -fsanitize=undefined"
    cxxflags="${cxxflags} -fsanitize=undefined"
    ldflags=" ${ldflags}  -fsanitize=undefined"
    driver_name="${driver_name}-ubsan"
    llvm_sanitizer="${llvm_sanitizer:+${llvm_sanitizer};}undefined"
fi

print_q   "Add coverage instrumentation?"
print_url "https://clang.llvm.org/docs/SourceBasedCodeCoverage.html"
print_nl

build_coverage=$( ask_user "y|n" "n" )
print_sel_yes_no "${build_coverage}"

if [[ "${build_coverage}" == "y" ]]; then
    cflags="  ${cflags}   -fprofile-instr-generate -fcoverage-mapping"
    cxxflags="${cxxflags} -fprofile-instr-generate -fcoverage-mapping"
    ldflags=" ${ldflags}  -fprofile-instr-generate"

    driver_name="${driver_name}-cov"
fi

if [[ "${build_san}" != "n"  ]]; then
    print_q    "Choose the optimisation level:"
    print_info "0" "compile with '-g -O0'"
    print_info "1" "compile with '-g -O1'"
    print_info "2" "compile with '-g -O2'"
    print_info "3" "compile with '-g -O3'"
    print_nl

    build_debugging=$( ask_user "0|1|2|3" "1" )
    print_sel "${build_debugging}" \
              "0" "-g -O0"         \
              "1" "-g -O1"         \
              "2" "-g -O2"         \
              "3" "-g -O3"
else
    print_q    "Add debugging flags?"
    print_info "0" "compile with '-g -O0'"
    print_info "1" "compile with '-g -O1'"
    print_info "2" "compile with '-g -O2'"
    print_info "3" "compile with '-g -O3'"
    print_info "n" "compile without debugging flags"
    print_nl

    build_debugging=$( ask_user "0|1|2|3|n" "1" )
    print_sel "${build_debugging}" \
              "0" "-g -O0"         \
              "1" "-g -O1"         \
              "2" "-g -O2"         \
              "3" "-g -O3"         \
              "n" "no"
fi

if [[ "${build_debugging}" != "n" ]]; then
    cflags="  ${cflags}   -g -O${build_debugging}"
    cxxflags="${cxxflags} -g -O${build_debugging}"

    driver_name="${driver_name}-o${build_debugging}"
fi

if [[ "${build_type}" == "f" ]]; then
    build_glog="n"
else
    print_q    "Use Glog logger?"
    print_url  "https://github.com/google/glog"
    print_info "no" "logging will be compiled out and glog will not be linked"
    print_nl

    build_glog=$( ask_user "y|n" "n" )
    print_sel_yes_no "${build_glog}"
fi

if [[ "${build_glog}" == "y" ]]; then
    export CMAKE_USE_LOGGER_GLOG=1

    driver_name="${driver_name}-glog"
fi

if [[ "${build_type}" == "f" ]]; then
    build_ft_trace="n"
else
    print_q    "Add FreeType tracing?"
    print_info "yes" "activate 'FT_DEBUG_LEVEL_{TRACE,ERROR}' and 'FT_DEBUG_{MEMORY,LOGGING}'"
    print_nl

    build_ft_trace=$( ask_user "y|n" "n" )
    print_sel_yes_no "${build_ft_trace}"
fi

if [[ "${build_ft_trace}" == "y" ]]; then
    cflags="  ${cflags}   -DFT_DEBUG_LEVEL_TRACE -DFT_DEBUG_LEVEL_ERROR -DFT_DEBUG_MEMORY -DFT_DEBUG_LOGGING -pthread"
    cxxflags="${cxxflags} -DFT_DEBUG_LEVEL_TRACE -DFT_DEBUG_LEVEL_ERROR -DFT_DEBUG_MEMORY -DFT_DEBUG_LOGGING -pthread"
    ldflags=" ${ldflags}  -pthread"

    driver_name="${driver_name}-fttrace"
fi

if ! command -v "ccache" >"/dev/null"; then
    build_ccache="n"
else
    print_q    "Use ccache?"
    print_info "ccache seems to be available on this system"
    print_nl

    build_ccache=$( ask_user "y|n" "y" )
    print_sel_yes_no "${build_ccache}"
fi

if [[ "${build_ccache}" == "y" ]]; then
    cc="ccache ${cc}"
    cxx="ccache ${cxx}"

    driver_name="${driver_name}-ccache"
fi

print_nl

# ----------------------------------------------------------------------------
# export flags and build everything:

export CC="${cc}"
export CXX="${cxx}"

export CFLAGS="${cflags}"
export CXXFLAGS="${cxxflags}"
export LDFLAGS="${ldflags}"

export SANITIZER="${llvm_sanitizer}"
export CMAKE_DRIVER_EXE_NAME="${driver_name}"

# Each project must be listed after any project it depends on.
bash "build/zlib.sh"
bash "build/bzip2.sh"
bash "build/libarchive.sh"
bash "build/brotli.sh"
bash "build/libpng.sh"
bash "build/freetype.sh"
LDFLAGS= bash "build/libcxx.sh"
if [[ "${build_glog}" == "y" ]]; then
    #TODO: use the static libcxx
    bash "build/glog.sh"
fi
bash "build/targets.sh"

cd "${dir}"
