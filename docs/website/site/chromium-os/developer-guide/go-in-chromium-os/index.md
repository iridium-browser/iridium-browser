---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/developer-guide
  - Chromium OS Developer Guide
page_name: go-in-chromium-os
title: Go in Chromium OS
---

[TOC]

This page provides information on using [Go](https://golang.org) in Chromium OS,
including recommendations for project organization, importing and managing third
party packages, and writing ebuilds.

## **Recommended organization of code for Go projects in Chromium OS**

Projects should generally follow the official recommendation to organize their
Go workspaces: [golang.org/doc/code.html](https://golang.org/doc/code.html)
These are some additional recommendations and naming conventions for organizing
Go project workspaces in Chromium OS:

*   If the Go project is in its own git repository, it should be set up
            such that the root of the repository is a workspace.
    *   It should have a single directory path `"src/<project name>/"`
                at the top level of the repository.
    *   This way, import paths for packages in the project always begin
                with `"<project name>/"`.
*   If the project provides useful Go library packages for other
            projects to import, put all such packages under `"src/chromiumos/"`.
    *   This way, import paths for Chromium OS specific common Go
                packages always begin with `"chromiumos/"` prefix.
*   When adding Go code to an existing project already containing code
            in other languages, create a top level directory called `"go"` in
            the repository and use that as the Go workspace.
    *   Follow the rest of the recommendation for new projects above
                (e.g. all code should be under `"go/src/<project>/"` or
                `"go/src/chromiumos/"`).
*   **IMPORTANT:** Do not use `"go get"` or vendor third party packages
            to individual project repositories.
    *   Instead, write an ebuild (in `chromiumos-overlay/dev-go`) that
                installs the third party packages to **`"/usr/lib/gopath"`**.

## **Recommendations for local development and builds**

Set the [GOPATH environment
variable](https://golang.org/cmd/go/#hdr-GOPATH_environment_variable) to contain
two elements:

1.  Path to the local Go workspace (directory containing
            `"src/<project>/"`).
2.  Path to all locally installed (emerged) Go packages:
            **`"/usr/lib/gopath"`**.

When compiling for host, invoke the Go command as `"x86_64-pc-linux-gnu-go"`, or
simply, `"go"`.

*   The host compiler is included in the chromiumos-sdk (`cros_sdk`) and
            is installed automatically when a chroot is created.
*   A few useful utilities like `"gofmt"`, `"godoc"`, `"golint"`, and
            `"goguru"` are also in the SDK and installed in the chroot.
    *   `"repo upload"` is configured to warn when trying to upload a
                change containing any Go files not formatted with `gofmt`.

Go cross compiler for a board is installed automatically when `"setup_board"` is
run in the chroot.

*   The cross-compiling Go command is determined by the toolchain target
            tuple for the board:
    *   Use `"x86_64-cros-linux-gnu-go"` for `amd64` targets.
    *   Use `"armv7a-cros-linux-gnueabi-go"` for `arm` targets.
    *   Use `"aarch64-cros-linux-gnu-go"` for `arm64` targets.
*   The cross compilers are configured to enable full Cgo support (i.e.
            they automatically use the correct cross-clang for compiling the C
            parts).

## **Setting up ebuilds for third party Go packages**

Write an ebuild to fetch and install third party packages to
**`"/usr/lib/gopath"`**.

*   If the upstream repository has tagged releases, use the ebuild
            version to fetch and install the corresponding release. Example:
            [dev-go/errors](https://chromium.googlesource.com/chromiumos/overlays/chromiumos-overlay/+/HEAD/dev-go/errors)

    ```none
    # Copyright 2018 The ChromiumOS Authors.
    # Distributed under the terms of the GNU General Public License v2.
    
    EAPI="6"
    
    CROS_GO_SOURCE="github.com/pkg/errors v${PV}"
    CROS_GO_PACKAGES=(
        	"github.com/pkg/errors"
    )
    
    inherit cros-go
    
    DESCRIPTION="Error handling primitives for Go."
    HOMEPAGE="https://github.com/pkg/errors"
    SRC_URI="$(cros-go_src_uri)"
    
    LICENSE="BSD"
    SLOT="0"
    KEYWORDS="*"
    IUSE=""
    RESTRICT="binchecks strip"
    
    DEPEND=""
    RDEPEND=""
    ```

*   If the upstream repository does not have release tags, keep the
            ebuild version at `0.0.1` and manage the commit hash manually.
            Example:
            [dev-go/glog](https://chromium.googlesource.com/chromiumos/overlays/chromiumos-overlay/+/HEAD/dev-go/glog/glog-0.0.1.ebuild)

    ```none
    # Copyright 2018 The ChromiumOS Authors.
    # Distributed under the terms of the GNU General Public License v2.

    EAPI="6"

    CROS_GO_SOURCE="github.com/golang/glog 44145f04b68cf362d9c4df2182967c2275eaefed"

    CROS_GO_PACKAGES=(
        "github.com/golang/glog"
    )

    inherit cros-go

    DESCRIPTION="Leveled execution logs for Go"
    HOMEPAGE="https://github.com/golang/glog"
    SRC_URI="$(cros-go_src_uri)"

    LICENSE="BSD-Google"
    SLOT="0"
    KEYWORDS="*"
    IUSE=""
    RESTRICT="binchecks strip"

    DEPEND=""
    RDEPEND=""
    ```

*   If the canonical import path is different from the repository path,
            specify it along with the repository path. Example:
            [dev-go/go-sys](https://chromium.googlesource.com/chromiumos/overlays/chromiumos-overlay/+/HEAD/dev-go/go-sys/go-sys-0.0.1.ebuild)

    ```none
    # Copyright 2018 The ChromiumOS Authors.
    # Distributed under the terms of the GNU General Public License v2.

    EAPI="6"

    CROS_GO_SOURCE="go.googlesource.com/sys:golang.org/x/sys a646d33e2ee3172a661fc09bca23bb4889a41bc8"

    CROS_GO_PACKAGES=(
        "golang.org/x/sys/unix"
    )

    inherit cros-go

    DESCRIPTION="Go packages for low-level interaction with the operating system"
    HOMEPAGE="https://golang.org/x/sys"
    SRC_URI="$(cros-go_src_uri)"

    LICENSE="BSD-Google"
    SLOT="0"
    KEYWORDS="*"
    IUSE=""
    RESTRICT="binchecks strip"

    DEPEND=""
    RDEPEND=""
    ```

*   Inherit eclass `"cros-go"`.
*   Use `CROS_GO_SOURCE` to specify the source repo, commit-id, and
            optionally, an alternate import path.
*   Use `CROS_GO_PACKAGES` to list the packages to install.
*   The installed packages can now be imported using their usual import
            paths, i.e.

    ```none
    import "golang.org/x/sys/unix"
    ```

*   Use `cros-go_src_uri()` to construct a valid `SRC_URI`.
*   The first time this ebuild is emerged, Portage will complain that
            the distfile could not be downloaded from gentoo-mirror or
            chromeos-localmirror.       This is expected, since we configure
            Portage to only look in the mirror locations and never download
            directly from `SRC_URI`.   The `cros-go.eclass` will automatically
            print a helpful list of commands for downloading the distfile and
            adding it to chromeos-localmirror:

    ```none
    >>> Emerging (1 of 1) dev-go/errors-0.8.0::chromiumos
    !!! Fetched file: github.com-pkg-errors-v0.8.0.tar.gz VERIFY FAILED!
    !!! Reason: Insufficient data for checksum verification
    !!! Got:
    !!! Expected: MD5 RMD160 SHA1 SHA256 SHA512 WHIRLPOOL
    * Fetch failed for 'dev-go/errors-0.8.0', Log file:
    *  '/var/log/portage/dev-go:errors-0.8.0:20180518-202435.log'
    * Run these commands to add github.com-pkg-errors-v0.8.0.tar.gz to chromeos-localmirror:
    *   wget -O github.com-pkg-errors-v0.8.0.tar.gz https://github.com/pkg/errors/archive/v0.8.0.tar.gz
    *   gsutil cp -a public-read github.com-pkg-errors-v0.8.0.tar.gz gs://chromeos-localmirror/distfiles/
    * and update the 'Manifest' file with:
    *   ebuild /mnt/host/source/src/third_party/chromiumos-overlay/dev-go/errors/errors-0.8.0.ebuild manifest
    ```

    Run the suggested `wget`, `gsutil`, and `ebuild` commands, and then try the
    emerge again.        For more information on `chromeos-localmirror` and
    setting up `gsutil`, [read
    this](https://sites.google.com/a/google.com/chromeos/resources/engineering/releng/localmirror).

## **Setting up ebuilds for Chromium OS Go projects**

**IMPORTANT:** Never use `"go get"` from an ebuild. Instead, write ebuilds for
external dependencies and let Portage make them available under
**`"/usr/lib/gopath"`**.
If a project is only providing common `"chromiumos/"` Go packages for use by
other projects, its ebuild only needs to fetch and install the package files to
**`"/usr/lib/gopath"`**.

*   Example:
            [dev-go/seccomp](https://chromium.googlesource.com/chromiumos/overlays/chromiumos-overlay/+/HEAD/dev-go/seccomp/seccomp-9999.ebuild)

    ```none
    # Copyright 2018 The ChromiumOS Authors.
    # Distributed under the terms of the GNU General Public License v2.

    EAPI="6"

    CROS_WORKON_PROJECT="chromiumos/platform/go-seccomp"
    CROS_WORKON_LOCALNAME="../platform/go-seccomp"

    CROS_GO_PACKAGES=(
        "chromiumos/seccomp"
    )

    inherit cros-workon cros-go

    DESCRIPTION="Go support for Chromium OS Seccomp-BPF policy files"
    HOMEPAGE="https://chromium.org/chromium-os/developer-guide/chromium-os-sandboxing"

    LICENSE="BSD-Google"
    SLOT="0"
    KEYWORDS="~*"
    IUSE=""
    RESTRICT="binchecks strip"

    DEPEND=""
    RDEPEND=""
    ```

    *   The seccomp package is located in
                `"chromiumos/platform/go-seccomp"` repo at
                `"src/chromiumos/seccomp/"`.
    *   This package can now be imported by other projects using

        ```none
        import "chromiumos/seccomp"
        ```

*   Note that a single ebuild can install multiple related packages
            inside `"/usr/lib/gopath/src/chromiumos/..."`.
*   Also, multiple ebuilds can install packages inside
            `"/usr/lib/gopath/src/chromiumos/..."` as long as there's no
            conflict for package paths.

For a typical Go project that needs to build and install executables:

*   Example:
            [dev-go/golint](https://chromium.googlesource.com/chromiumos/overlays/chromiumos-overlay/+/HEAD/dev-go/golint/golint-0.0.1.ebuild)

    ```none
    # Copyright 2018 The ChromiumOS Authors.
    # Distributed under the terms of the GNU General Public License v2.

    EAPI="6"

    CROS_GO_SOURCE="github.com/golang/lint c5fb716d6688a859aae56d26d3e6070808df29f7"

    CROS_GO_BINARIES=(
        "github.com/golang/lint/golint"
    )

    inherit cros-go

    DESCRIPTION="A linter for Go source code"
    HOMEPAGE="https://github.com/golang/lint"
    SRC_URI="$(cros-go_src_uri)"

    LICENSE="BSD-Google"
    SLOT="0"
    KEYWORDS="*"
    IUSE=""
    RESTRICT="binchecks strip"

    DEPEND="dev-go/go-tools"
    RDEPEND=""
    ```

*   If the project needs to import packages from outside its own
            repository, list those dependencies in the `DEPEND` variable.
*   Always specify the binaries using `"CROS_GO_BINARIES"` variable.
            This will pick the correct compiler for cross-compiling, and invoke
            it with appropriate `GOPATH` and flags.     (e.g. `"emerge-daisy"`
            will automatically use the arm cross-compiler, import packages from
            `"/build/daisy/usr/lib/gopath"`, and build PIE binaries).

A single ebuild can install executable binaries, as well as provide Go packages
for other projects to import.

*   Example:
            [dev-go/go-tools](https://chromium.googlesource.com/chromiumos/overlays/chromiumos-overlay/+/HEAD/dev-go/go-tools/go-tools-0.0.1.ebuild)

    ```none
    # Copyright 2018 The ChromiumOS Authors.
    # Distributed under the terms of the GNU General Public License v2.

    EAPI="6"

    CROS_GO_SOURCE="go.googlesource.com/tools:golang.org/x/tools 2d19ab38faf14664c76088411c21bf4fafea960b"

    CROS_GO_PACKAGES=(
        "golang.org/x/tools/go/gcimporter15"
        "golang.org/x/tools/go/gcexportdata"
    )

    CROS_GO_TEST=(
        "${CROS_GO_PACKAGES[@]}"
    )

    CROS_GO_BINARIES=(
        "golang.org/x/tools/cmd/godoc"
        "golang.org/x/tools/cmd/guru:goguru"
    )

    inherit cros-go

    DESCRIPTION="Packages and tools that support the Go programming language"
    HOMEPAGE="https://golang.org/x/tools"
    SRC_URI="$(cros-go_src_uri)"

    LICENSE="BSD-Google"
    SLOT="0"
    KEYWORDS="*"
    IUSE=""
    RESTRICT="binchecks strip"

    DEPEND=""
    RDEPEND=""
    ```

    *   This builds and installs the `"godoc"` and `"goguru"` tools.
                It also provides packages `"golang.org/x/tools/go/gcimporter15"`
                and `"golang.org/x/tools/go/gcexportdata"` required to build
                `"dev-go/golint"` above.

## **Upgrading ebuilds for third party Go projects**

When we need to upgrade a newer version of an existing third party package,
simply update the ebuild files and the Manifest.

* If the upstream has a released version.
  - Rename the ebuild files to track the
newer version number. Here is an example:
    ```shell
    export OPV=0.3.7
    export NPV=0.4.0
    # Rename the ebuild file to track the new version
    mv dev-go/text/text-${OPV}.ebuild dev-go/text-${NPV}.ebuild
    # Create a symlinked ebuild file with revision number.
    ln -sf dev-go/text-${NPV}.ebuild dev-go/text-${NPV}-r1.ebuild
    # Delete any old ebuild with a revision number.
    rm dev-go/text/text-${OPV}-r3.ebuild

    ```
  - Update the Manifest file through `ebuild` command.
    ```shell
    ebuild dev-go/text/text-${NPV}.ebuild manifest
    ```
    This will fail for the first time because there is no distfile ready to be
    downloaded from the chromeos-localmirror. Run the suggested `wget`,
    `gsutil` and `ebuild` commands.
  - Emerge with the package and make sure it is successful.
  - Commit the change and upload the cl.

* If the upstream has no release vesion and the ebuild is set up to track
  specific commit.

  - Update `CROS_GO_SOURCE` in the ebuild file to track the desired commit hash.
  - Bump the revision number.
  - Update Manifest file through `ebuild` command. Upload the new distfile as
    instructed by the first `ebuild` run.
  - Emerge with the package and make sure it is successful.
  - Commit the change and upload the cl.

## **Location of ebuilds and repositories**

*   Ebuilds for third party Go packages and tools should go in
            `chromiumos-overlay/dev-go`.
*   Ebuilds for Chromium OS specific common Go packages (import paths
            beginning with `"chromiumos/"`) should also go to
            `chromiumos-overlay/dev-go`. Their repositories should go to an
            appropriate place in the tree (e.g.
            "[chromium.googlesource.com/chromiumos/platform/go-seccomp](https://chromium.googlesource.com/chromiumos/platform/go-seccomp)").
*   For other Chromium OS Go projects, use `chromiumos-overlay/dev-go`
            only if it's a generic helper tool or utility for development.
            Otherwise, pick a more appropriate category and overlay for the
            project.

## **Useful functions and variables**

The cros-go eclass is defined here:
[chromiumos-overlay/eclass/cros-go.eclass](https://chromium.googlesource.com/chromiumos/overlays/chromiumos-overlay/+/HEAD/eclass/cros-go.eclass)
`CROS_GO_SOURCE`                Path to the upstream repository and commit id.
Go repositories on `"github.com"` and `"*.googlesource.com"` are supported.
The source string contains the path of the git repository containing Go
packages, and a commit-id (or version tag).
For example:

```none
CROS_GO_SOURCE="github.com/golang/glog 44145f04b68cf362d9c4df2182967c2275eaefed"
```

will fetch the sources from
[github.com/golang/glog](https://github.com/golang/glog) at the specified
commit-id, and

```none
CROS_GO_SOURCE="github.com/pkg/errors v0.8.0"
```

will fetch the sources from
[github.com/pkg/errors](https://github.com/pkg/errors) at version `v0.8.0`.
By default, the import path for Go packages in the repository is the same as
repository path. This can be overridden by appending a colon to the repository
path, followed by an alternate import path.
For example:

```none
CROS_GO_SOURCE="github.com/go-yaml/yaml:gopkg.in/yaml.v2 v2.2.1"
```

will fetch the sources from
[github.com/go-yaml/yaml](https://github.com/go-yaml/yaml) at the version
`v2.2.1`, and install the package under `"gopkg.in/yaml.v2"`.
`CROS_GO_SOURCE` can contain multiple items when defined as an array:

```none
CROS_GO_SOURCE=(
    "github.com/golang/glog 44145f04b68cf362d9c4df2182967c2275eaefed"
    "github.com/pkg/errors v0.8.0"
    "github.com/go-yaml/yaml:gopkg.in/yaml.v2 v2.2.1"
)
```

`CROS_GO_WORKSPACE`             Path to the Go workspace, default is `"${S}"`.
The Go workspace is searched for packages to build and install.  If all Go
packages in the repository are under `"go/src/"`:

```none
CROS_GO_WORKSPACE="${S}/go"
```

`CROS_GO_WORKSPACE` can contain multiple items when defined as an array:

```none
CROS_GO_WORKSPACE=(
    "${S}"
    "${S}/tast-base"
)
```

`CROS_GO_BINARIES`              Go executable binaries to build and install.
Each path must contain a `package "main"`. The last component of the package
path will become the name of the executable.       The executable name can be
overridden by appending a colon to the package path, followed by an alternate
name.  The install path for an executable can be overridden by appending a colon
to the package path, followed by the desired install path/name for it.
For example:

```none
CROS_GO_BINARIES=(
    "golang.org/x/tools/cmd/godoc"
    "golang.org/x/tools/cmd/guru:goguru"
    "golang.org/x/tools/cmd/stringer:/usr/local/bin/gostringer"
)
```

will build and install `"godoc"`, `"goguru"`, and `"gostringer"` binaries.
`CROS_GO_VERSION`               Version string to embed in the executable
binary. The variable `main.Version` is set to this value at build time.
For example:

```none
CROS_GO_VERSION="${PVR}"
```

will set `main.Version` string variable to package version and revision (if any)
of the ebuild.
`CROS_GO_PACKAGES`              Go packages to install in
**`"/usr/lib/gopath"`**.      Packages are installed in **`"/usr/lib/gopath"`**
such that they can be imported later from Go code using the exact paths listed
here.
For example:

```none
CROS_GO_PACKAGES=(	"github.com/golang/glog")
```

will install package files to `"/usr/lib/gopath/src/github.com/golang/glog"` and
other Go projects can use the package with

```none
import "github.com/golang/glog"
```

If the last component of a package path is `"..."`, it is expanded to include
all Go packages under the directory.
`CROS_GO_TEST`          Go packages to test.    Package tests are run with
`-short` flag by default.    Package tests are always built and run locally on
host.         Default is to test all packages in `CROS_GO_WORKSPACE`(s).
`cros-go_src_uri`               Construct a valid `SRC_URI` from
`CROS_GO_SOURCE`. Set the `SRC_URI` in an ebuild with:

```none
SRC_URI="$(cros-go_src_uri)"
```

`cros-go_gopath`                A valid `GOPATH` for `CROS_GO_WORKSPACE`. Set
the `GOPATH` in an ebuild with:

```none
GOPATH="$(cros-go_gopath)"
```

`cros_go`               Wrapper function for invoking the Go tool from an
ebuild. It provides the following functionality:

*   The correct cross-compiler is automatically detected and used. For
            example, when running `"emerge-daisy"`,
            `"armv7a-cros-linux-gnueabi-go"` will be used to build Go packages
            and binaries.
*   The `GOPATH` variable is setup automatically. For example, when
            running `"emerge-daisy"`, Go packages will be looked up in the local
            workspace, as well as in `"/build/daisy/usr/lib/gopath"`.
*   When cross-compiling Go packages that use Cgo, the correct C/C++
            cross-compilers for the target are automatically detected and used.

For most ebuilds, setting the `CROS_GO_BINARIES` variable should be enough to
build and install Go binaries. Implementation of `CROS_GO_BINARIES` uses the
`cros_go` wrapper internally.
