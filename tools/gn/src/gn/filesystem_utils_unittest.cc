// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gn/filesystem_utils.h"

#include <thread>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "gn/target.h"
#include "util/build_config.h"
#include "util/test/test.h"

TEST(FilesystemUtils, FileExtensionOffset) {
  EXPECT_EQ(std::string::npos, FindExtensionOffset(""));
  EXPECT_EQ(std::string::npos, FindExtensionOffset("foo/bar/baz"));
  EXPECT_EQ(4u, FindExtensionOffset("foo."));
  EXPECT_EQ(4u, FindExtensionOffset("f.o.bar"));
  EXPECT_EQ(std::string::npos, FindExtensionOffset("foo.bar/"));
  EXPECT_EQ(std::string::npos, FindExtensionOffset("foo.bar/baz"));
}

TEST(FilesystemUtils, FindExtension) {
  std::string input;
  EXPECT_EQ("", FindExtension(&input));
  input = "foo/bar/baz";
  EXPECT_EQ("", FindExtension(&input));
  input = "foo.";
  EXPECT_EQ("", FindExtension(&input));
  input = "f.o.bar";
  EXPECT_EQ("bar", FindExtension(&input));
  input = "foo.bar/";
  EXPECT_EQ("", FindExtension(&input));
  input = "foo.bar/baz";
  EXPECT_EQ("", FindExtension(&input));
}

TEST(FilesystemUtils, FindFilenameOffset) {
  EXPECT_EQ(0u, FindFilenameOffset(""));
  EXPECT_EQ(0u, FindFilenameOffset("foo"));
  EXPECT_EQ(4u, FindFilenameOffset("foo/"));
  EXPECT_EQ(4u, FindFilenameOffset("foo/bar"));
}

TEST(FilesystemUtils, RemoveFilename) {
  std::string s;

  RemoveFilename(&s);
  EXPECT_STREQ("", s.c_str());

  s = "foo";
  RemoveFilename(&s);
  EXPECT_STREQ("", s.c_str());

  s = "/";
  RemoveFilename(&s);
  EXPECT_STREQ("/", s.c_str());

  s = "foo/bar";
  RemoveFilename(&s);
  EXPECT_STREQ("foo/", s.c_str());

  s = "foo/bar/baz.cc";
  RemoveFilename(&s);
  EXPECT_STREQ("foo/bar/", s.c_str());
}

TEST(FilesystemUtils, FindDir) {
  std::string input;
  EXPECT_EQ("", FindDir(&input));
  input = "/";
  EXPECT_EQ("/", FindDir(&input));
  input = "foo/";
  EXPECT_EQ("foo/", FindDir(&input));
  input = "foo/bar/baz";
  EXPECT_EQ("foo/bar/", FindDir(&input));
}

TEST(FilesystemUtils, FindLastDirComponent) {
  SourceDir empty;
  EXPECT_EQ("", FindLastDirComponent(empty));

  SourceDir root("/");
  EXPECT_EQ("", FindLastDirComponent(root));

  SourceDir srcroot("//");
  EXPECT_EQ("", FindLastDirComponent(srcroot));

  SourceDir regular1("//foo/");
  EXPECT_EQ("foo", FindLastDirComponent(regular1));

  SourceDir regular2("//foo/bar/");
  EXPECT_EQ("bar", FindLastDirComponent(regular2));
}

TEST(FilesystemUtils, EnsureStringIsInOutputDir) {
  SourceDir output_dir("//out/Debug/");

  // Some outside.
  Err err;
  EXPECT_FALSE(EnsureStringIsInOutputDir(output_dir, "//foo", nullptr, &err));
  EXPECT_TRUE(err.has_error());
  err = Err();
  EXPECT_FALSE(
      EnsureStringIsInOutputDir(output_dir, "//out/Debugit", nullptr, &err));
  EXPECT_TRUE(err.has_error());

  // Some inside.
  err = Err();
  EXPECT_TRUE(
      EnsureStringIsInOutputDir(output_dir, "//out/Debug/", nullptr, &err));
  EXPECT_FALSE(err.has_error());
  EXPECT_TRUE(
      EnsureStringIsInOutputDir(output_dir, "//out/Debug/foo", nullptr, &err));
  EXPECT_FALSE(err.has_error());

  // Pattern but no template expansions are allowed.
  EXPECT_FALSE(EnsureStringIsInOutputDir(output_dir, "{{source_gen_dir}}",
                                         nullptr, &err));
  EXPECT_TRUE(err.has_error());
}

TEST(FilesystemUtils, IsPathAbsolute) {
  EXPECT_TRUE(IsPathAbsolute("/foo/bar"));
  EXPECT_TRUE(IsPathAbsolute("/"));
  EXPECT_FALSE(IsPathAbsolute(""));
  EXPECT_FALSE(IsPathAbsolute("//"));
  EXPECT_FALSE(IsPathAbsolute("//foo/bar"));

#if defined(OS_WIN)
  EXPECT_TRUE(IsPathAbsolute("C:/foo"));
  EXPECT_TRUE(IsPathAbsolute("C:/"));
  EXPECT_TRUE(IsPathAbsolute("C:\\foo"));
  EXPECT_TRUE(IsPathAbsolute("C:\\"));
  EXPECT_TRUE(IsPathAbsolute("/C:/foo"));
  EXPECT_TRUE(IsPathAbsolute("/C:\\foo"));
#endif
}

TEST(FilesystemUtils, MakeAbsolutePathRelativeIfPossible) {
  std::string dest;

#if defined(OS_WIN)
  EXPECT_TRUE(
      MakeAbsolutePathRelativeIfPossible("C:\\base", "C:\\base\\foo", &dest));
  EXPECT_EQ("//foo", dest);
  EXPECT_TRUE(
      MakeAbsolutePathRelativeIfPossible("C:\\base", "/C:/base/foo", &dest));
  EXPECT_EQ("//foo", dest);
  EXPECT_TRUE(
      MakeAbsolutePathRelativeIfPossible("c:\\base", "C:\\base\\foo\\", &dest));
  EXPECT_EQ("//foo\\", dest);

  EXPECT_FALSE(MakeAbsolutePathRelativeIfPossible("C:\\base", "C:\\ba", &dest));
  EXPECT_FALSE(MakeAbsolutePathRelativeIfPossible("C:\\base",
                                                  "C:\\/notbase/foo", &dest));
#else

  EXPECT_TRUE(MakeAbsolutePathRelativeIfPossible("/base", "/base/foo/", &dest));
  EXPECT_EQ("//foo/", dest);
  EXPECT_TRUE(MakeAbsolutePathRelativeIfPossible("/base", "/base/foo", &dest));
  EXPECT_EQ("//foo", dest);
  EXPECT_TRUE(
      MakeAbsolutePathRelativeIfPossible("/base/", "/base/foo/", &dest));
  EXPECT_EQ("//foo/", dest);

  EXPECT_FALSE(MakeAbsolutePathRelativeIfPossible("/base", "/ba", &dest));
  EXPECT_FALSE(
      MakeAbsolutePathRelativeIfPossible("/base", "/notbase/foo", &dest));
#endif
}

TEST(FilesystemUtils, MakeAbsoluteFilePathRelativeIfPossible) {
#if defined(OS_WIN)
  EXPECT_EQ(
      base::FilePath(u"out\\Debug"),
      MakeAbsoluteFilePathRelativeIfPossible(
          base::FilePath(u"C:\\src"), base::FilePath(u"C:\\src\\out\\Debug")));
  EXPECT_EQ(base::FilePath(u".\\gn"),
            MakeAbsoluteFilePathRelativeIfPossible(
                base::FilePath(u"C:\\src\\out\\Debug"),
                base::FilePath(u"C:\\src\\out\\Debug\\gn")));
  EXPECT_EQ(
      base::FilePath(u"..\\.."),
      MakeAbsoluteFilePathRelativeIfPossible(
          base::FilePath(u"C:\\src\\out\\Debug"), base::FilePath(u"C:\\src")));
  EXPECT_EQ(
      base::FilePath(u"..\\.."),
      MakeAbsoluteFilePathRelativeIfPossible(
          base::FilePath(u"C:\\src\\out\\Debug"), base::FilePath(u"C:/src")));
  EXPECT_EQ(base::FilePath(u"."),
            MakeAbsoluteFilePathRelativeIfPossible(base::FilePath(u"C:\\src"),
                                                   base::FilePath(u"C:\\src")));
  EXPECT_EQ(base::FilePath(u"..\\..\\..\\u\\v\\w"),
            MakeAbsoluteFilePathRelativeIfPossible(
                base::FilePath(u"C:\\a\\b\\c\\x\\y\\z"),
                base::FilePath(u"C:\\a\\b\\c\\u\\v\\w")));
  EXPECT_EQ(base::FilePath(u"D:\\bar"),
            MakeAbsoluteFilePathRelativeIfPossible(base::FilePath(u"C:\\foo"),
                                                   base::FilePath(u"D:\\bar")));
#else
  EXPECT_EQ(base::FilePath("out/Debug"),
            MakeAbsoluteFilePathRelativeIfPossible(
                base::FilePath("/src"), base::FilePath("/src/out/Debug")));
  EXPECT_EQ(base::FilePath("./gn"), MakeAbsoluteFilePathRelativeIfPossible(
                                        base::FilePath("/src/out/Debug"),
                                        base::FilePath("/src/out/Debug/gn")));
  EXPECT_EQ(base::FilePath("../.."),
            MakeAbsoluteFilePathRelativeIfPossible(
                base::FilePath("/src/out/Debug"), base::FilePath("/src")));
  EXPECT_EQ(base::FilePath("."),
            MakeAbsoluteFilePathRelativeIfPossible(base::FilePath("/src"),
                                                   base::FilePath("/src")));
  EXPECT_EQ(
      base::FilePath("../../../u/v/w"),
      MakeAbsoluteFilePathRelativeIfPossible(base::FilePath("/a/b/c/x/y/z"),
                                             base::FilePath("/a/b/c/u/v/w")));
#endif
}

TEST(FilesystemUtils, NormalizePath) {
  std::string input;

  NormalizePath(&input);
  EXPECT_EQ("", input);

  input = "foo/bar.txt";
  NormalizePath(&input);
  EXPECT_EQ("foo/bar.txt", input);

  input = ".";
  NormalizePath(&input);
  EXPECT_EQ("", input);

  input = "..";
  NormalizePath(&input);
  EXPECT_EQ("..", input);

  input = "foo//bar";
  NormalizePath(&input);
  EXPECT_EQ("foo/bar", input);

  input = "//foo";
  NormalizePath(&input);
  EXPECT_EQ("//foo", input);

  input = "foo/..//bar";
  NormalizePath(&input);
  EXPECT_EQ("bar", input);

  input = "foo/../../bar";
  NormalizePath(&input);
  EXPECT_EQ("../bar", input);

  input = "/../foo";  // Don't go above the root dir.
  NormalizePath(&input);
  EXPECT_EQ("/foo", input);

  input = "//../foo";  // Don't go above the root dir.
  NormalizePath(&input);
  EXPECT_EQ("//foo", input);

  input = "../foo";
  NormalizePath(&input);
  EXPECT_EQ("../foo", input);

  input = "..";
  NormalizePath(&input);
  EXPECT_EQ("..", input);

  input = "./././.";
  NormalizePath(&input);
  EXPECT_EQ("", input);

  input = "../../..";
  NormalizePath(&input);
  EXPECT_EQ("../../..", input);

  input = "../";
  NormalizePath(&input);
  EXPECT_EQ("../", input);

  // Backslash normalization.
  input = "foo\\..\\..\\bar";
  NormalizePath(&input);
  EXPECT_EQ("../bar", input);

  // Trailing slashes should get preserved.
  input = "//foo/bar/";
  NormalizePath(&input);
  EXPECT_EQ("//foo/bar/", input);

#if defined(OS_WIN)
  // Go above and outside of the source root.
  input = "//../foo";
  NormalizePath(&input, "/C:/source/root");
  EXPECT_EQ("/C:/source/foo", input);

  input = "//../foo";
  NormalizePath(&input, "C:\\source\\root");
  EXPECT_EQ("/C:/source/foo", input);

  input = "//../";
  NormalizePath(&input, "/C:/source/root");
  EXPECT_EQ("/C:/source/", input);

  input = "//../foo.txt";
  NormalizePath(&input, "/C:/source/root");
  EXPECT_EQ("/C:/source/foo.txt", input);

  input = "//../foo/bar/";
  NormalizePath(&input, "/C:/source/root");
  EXPECT_EQ("/C:/source/foo/bar/", input);

  // Go above and back into the source root. This should return a system-
  // absolute path. We could arguably return this as a source-absolute path,
  // but that would require additional handling to account for a rare edge
  // case.
  input = "//../root/foo";
  NormalizePath(&input, "/C:/source/root");
  EXPECT_EQ("/C:/source/root/foo", input);

  input = "//../root/foo/bar/";
  NormalizePath(&input, "/C:/source/root");
  EXPECT_EQ("/C:/source/root/foo/bar/", input);

  // Stay inside the source root
  input = "//foo/bar";
  NormalizePath(&input, "/C:/source/root");
  EXPECT_EQ("//foo/bar", input);

  input = "//foo/bar/";
  NormalizePath(&input, "/C:/source/root");
  EXPECT_EQ("//foo/bar/", input);

  // The path should not go above the system root. Note that on Windows, this
  // will consume the drive (C:).
  input = "//../../../../../foo/bar";
  NormalizePath(&input, "/C:/source/root");
  EXPECT_EQ("/foo/bar", input);

  // Test when the source root is the letter drive.
  input = "//../foo";
  NormalizePath(&input, "/C:");
  EXPECT_EQ("/foo", input);

  input = "//../foo";
  NormalizePath(&input, "C:");
  EXPECT_EQ("/foo", input);

  input = "//../foo";
  NormalizePath(&input, "/");
  EXPECT_EQ("/foo", input);

  input = "//../";
  NormalizePath(&input, "\\C:");
  EXPECT_EQ("/", input);

  input = "//../foo.txt";
  NormalizePath(&input, "/C:");
  EXPECT_EQ("/foo.txt", input);
#else
  // Go above and outside of the source root.
  input = "//../foo";
  NormalizePath(&input, "/source/root");
  EXPECT_EQ("/source/foo", input);

  input = "//../";
  NormalizePath(&input, "/source/root");
  EXPECT_EQ("/source/", input);

  input = "//../foo.txt";
  NormalizePath(&input, "/source/root");
  EXPECT_EQ("/source/foo.txt", input);

  input = "//../foo/bar/";
  NormalizePath(&input, "/source/root");
  EXPECT_EQ("/source/foo/bar/", input);

  // Go above and back into the source root. This should return a system-
  // absolute path. We could arguably return this as a source-absolute path,
  // but that would require additional handling to account for a rare edge
  // case.
  input = "//../root/foo";
  NormalizePath(&input, "/source/root");
  EXPECT_EQ("/source/root/foo", input);

  input = "//../root/foo/bar/";
  NormalizePath(&input, "/source/root");
  EXPECT_EQ("/source/root/foo/bar/", input);

  // Stay inside the source root
  input = "//foo/bar";
  NormalizePath(&input, "/source/root");
  EXPECT_EQ("//foo/bar", input);

  input = "//foo/bar/";
  NormalizePath(&input, "/source/root");
  EXPECT_EQ("//foo/bar/", input);

  // The path should not go above the system root.
  input = "//../../../../../foo/bar";
  NormalizePath(&input, "/source/root");
  EXPECT_EQ("/foo/bar", input);

  // Test when the source root is the system root.
  input = "//../foo/bar/";
  NormalizePath(&input, "/");
  EXPECT_EQ("/foo/bar/", input);

  input = "//../";
  NormalizePath(&input, "/");
  EXPECT_EQ("/", input);

  input = "//../foo.txt";
  NormalizePath(&input, "/");
  EXPECT_EQ("/foo.txt", input);

#endif
}

TEST(FilesystemUtils, RebasePath) {
  std::string_view source_root("/source/root");

  // Degenerate case.
  EXPECT_EQ(".", RebasePath("//", SourceDir("//"), source_root));
  EXPECT_EQ(".",
            RebasePath("//foo/bar/", SourceDir("//foo/bar/"), source_root));

  // Going up the tree.
  EXPECT_EQ("../foo", RebasePath("//foo", SourceDir("//bar/"), source_root));
  EXPECT_EQ("../foo/", RebasePath("//foo/", SourceDir("//bar/"), source_root));
  EXPECT_EQ("../../foo",
            RebasePath("//foo", SourceDir("//bar/moo"), source_root));
  EXPECT_EQ("../../foo/",
            RebasePath("//foo/", SourceDir("//bar/moo"), source_root));

  // Going down the tree.
  EXPECT_EQ("foo/bar", RebasePath("//foo/bar", SourceDir("//"), source_root));
  EXPECT_EQ("foo/bar/", RebasePath("//foo/bar/", SourceDir("//"), source_root));

  // Going up and down the tree.
  EXPECT_EQ("../../foo/bar",
            RebasePath("//foo/bar", SourceDir("//a/b/"), source_root));
  EXPECT_EQ("../../foo/bar/",
            RebasePath("//foo/bar/", SourceDir("//a/b/"), source_root));

  // Sharing prefix.
  EXPECT_EQ("foo", RebasePath("//a/foo", SourceDir("//a/"), source_root));
  EXPECT_EQ("foo", RebasePath("//a/foo", SourceDir("//a"), source_root));
  EXPECT_EQ("foo/", RebasePath("//a/foo/", SourceDir("//a/"), source_root));
  EXPECT_EQ("foo", RebasePath("//a/b/foo", SourceDir("//a/b/"), source_root));
  EXPECT_EQ("foo/", RebasePath("//a/b/foo/", SourceDir("//a/b/"), source_root));
  EXPECT_EQ("foo/bar",
            RebasePath("//a/b/foo/bar", SourceDir("//a/b/"), source_root));
  EXPECT_EQ("foo/bar/",
            RebasePath("//a/b/foo/bar/", SourceDir("//a/b/"), source_root));
  EXPECT_EQ(".",
            RebasePath("//foo/bar", SourceDir("//foo/bar/"), source_root));
  EXPECT_EQ("..",
            RebasePath("//foo", SourceDir("//foo/bar/"), source_root));
  EXPECT_EQ("../",
            RebasePath("//foo/", SourceDir("//foo/bar/"), source_root));

  // Check when only |input| is system-absolute
  EXPECT_EQ("foo", RebasePath("/source/root/foo", SourceDir("//"),
                              std::string_view("/source/root")));
  EXPECT_EQ("foo/", RebasePath("/source/root/foo/", SourceDir("//"),
                               std::string_view("/source/root")));
  EXPECT_EQ("../../builddir/Out/Debug",
            RebasePath("/builddir/Out/Debug", SourceDir("//"),
                       std::string_view("/source/root")));
  EXPECT_EQ("../../../builddir/Out/Debug",
            RebasePath("/builddir/Out/Debug", SourceDir("//"),
                       std::string_view("/source/root/foo")));
  EXPECT_EQ("../../../builddir/Out/Debug/",
            RebasePath("/builddir/Out/Debug/", SourceDir("//"),
                       std::string_view("/source/root/foo")));
  EXPECT_EQ("../../path/to/foo", RebasePath("/path/to/foo", SourceDir("//"),
                                            std::string_view("/source/root")));
  EXPECT_EQ("../../../path/to/foo",
            RebasePath("/path/to/foo", SourceDir("//a"),
                       std::string_view("/source/root")));
  EXPECT_EQ("../../../../path/to/foo",
            RebasePath("/path/to/foo", SourceDir("//a/b"),
                       std::string_view("/source/root")));

  // Check when only |dest_dir| is system-absolute.
  EXPECT_EQ(".", RebasePath("//", SourceDir("/source/root"),
                            std::string_view("/source/root")));
  EXPECT_EQ("foo", RebasePath("//foo", SourceDir("/source/root"),
                              std::string_view("/source/root")));
  EXPECT_EQ("../foo", RebasePath("//foo", SourceDir("/source/root/bar"),
                                 std::string_view("/source/root")));
  EXPECT_EQ("../../../source/root/foo",
            RebasePath("//foo", SourceDir("/other/source/root"),
                       std::string_view("/source/root")));
  EXPECT_EQ("../../../../source/root/foo",
            RebasePath("//foo", SourceDir("/other/source/root/bar"),
                       std::string_view("/source/root")));

  // Check when |input| and |dest_dir| are both system-absolute. Also,
  // in this case |source_root| is never used so set it to a dummy
  // value.
  EXPECT_EQ("foo", RebasePath("/source/root/foo", SourceDir("/source/root"),
                              std::string_view("/x/y/z")));
  EXPECT_EQ("foo/", RebasePath("/source/root/foo/", SourceDir("/source/root"),
                               std::string_view("/x/y/z")));
  EXPECT_EQ("../../builddir/Out/Debug",
            RebasePath("/builddir/Out/Debug", SourceDir("/source/root"),
                       std::string_view("/x/y/z")));
  EXPECT_EQ("../../../builddir/Out/Debug",
            RebasePath("/builddir/Out/Debug", SourceDir("/source/root/foo"),
                       std::string_view("/source/root/foo")));
  EXPECT_EQ("../../../builddir/Out/Debug/",
            RebasePath("/builddir/Out/Debug/", SourceDir("/source/root/foo"),
                       std::string_view("/source/root/foo")));
  EXPECT_EQ("../../path/to/foo",
            RebasePath("/path/to/foo", SourceDir("/source/root"),
                       std::string_view("/x/y/z")));
  EXPECT_EQ("../../../path/to/foo",
            RebasePath("/path/to/foo", SourceDir("/source/root/a"),
                       std::string_view("/x/y/z")));
  EXPECT_EQ("../../../../path/to/foo",
            RebasePath("/path/to/foo", SourceDir("/source/root/a/b"),
                       std::string_view("/x/y/z")));

#if defined(OS_WIN)
  // Test corrections while rebasing Windows-style absolute paths.
  EXPECT_EQ("../../../../path/to/foo",
            RebasePath("C:/path/to/foo", SourceDir("//a/b"),
                       std::string_view("/C:/source/root")));
  EXPECT_EQ("../../../../path/to/foo",
            RebasePath("/C:/path/to/foo", SourceDir("//a/b"),
                       std::string_view("C:/source/root")));
  EXPECT_EQ("../../../../path/to/foo",
            RebasePath("/C:/path/to/foo", SourceDir("//a/b"),
                       std::string_view("/c:/source/root")));
  EXPECT_EQ("../../../../path/to/foo",
            RebasePath("/c:/path/to/foo", SourceDir("//a/b"),
                       std::string_view("c:/source/root")));
  EXPECT_EQ("../../../../path/to/foo",
            RebasePath("/c:/path/to/foo", SourceDir("//a/b"),
                       std::string_view("C:/source/root")));
#endif
}

TEST(FilesystemUtils, DirectoryWithNoLastSlash) {
  EXPECT_EQ("", DirectoryWithNoLastSlash(SourceDir()));
  EXPECT_EQ("/.", DirectoryWithNoLastSlash(SourceDir("/")));
  EXPECT_EQ("//.", DirectoryWithNoLastSlash(SourceDir("//")));
  EXPECT_EQ("//foo", DirectoryWithNoLastSlash(SourceDir("//foo/")));
  EXPECT_EQ("/bar", DirectoryWithNoLastSlash(SourceDir("/bar/")));
}

TEST(FilesystemUtils, SourceDirForPath) {
#if defined(OS_WIN)
  base::FilePath root(u"C:\\source\\foo\\");
  EXPECT_EQ("/C:/foo/bar/",
            SourceDirForPath(root, base::FilePath(u"C:\\foo\\bar")).value());
  EXPECT_EQ("/", SourceDirForPath(root, base::FilePath(u"/")).value());
  EXPECT_EQ("//",
            SourceDirForPath(root, base::FilePath(u"C:\\source\\foo")).value());
  EXPECT_EQ("//bar/",
            SourceDirForPath(root, base::FilePath(u"C:\\source\\foo\\bar\\"))
                .value());
  EXPECT_EQ("//bar/baz/",
            SourceDirForPath(root, base::FilePath(u"C:\\source\\foo\\bar\\baz"))
                .value());

  // Should be case-and-slash-insensitive.
  EXPECT_EQ(
      "//baR/",
      SourceDirForPath(root, base::FilePath(u"c:/SOURCE\\Foo/baR/")).value());

  // Some "weird" Windows paths.
  EXPECT_EQ("/foo/bar/",
            SourceDirForPath(root, base::FilePath(u"/foo/bar/")).value());
  EXPECT_EQ("/C:/foo/bar/",
            SourceDirForPath(root, base::FilePath(u"C:foo/bar/")).value());

  // Also allow absolute GN-style Windows paths.
  EXPECT_EQ("/C:/foo/bar/",
            SourceDirForPath(root, base::FilePath(u"/C:/foo/bar")).value());
  EXPECT_EQ(
      "//bar/",
      SourceDirForPath(root, base::FilePath(u"/C:/source/foo/bar")).value());

  // Empty source dir.
  base::FilePath empty;
  EXPECT_EQ(
      "/C:/source/foo/",
      SourceDirForPath(empty, base::FilePath(u"C:\\source\\foo")).value());
#else
  base::FilePath root("/source/foo/");
  EXPECT_EQ("/foo/bar/",
            SourceDirForPath(root, base::FilePath("/foo/bar/")).value());
  EXPECT_EQ("/", SourceDirForPath(root, base::FilePath("/")).value());
  EXPECT_EQ("//",
            SourceDirForPath(root, base::FilePath("/source/foo")).value());
  EXPECT_EQ("//bar/",
            SourceDirForPath(root, base::FilePath("/source/foo/bar/")).value());
  EXPECT_EQ(
      "//bar/baz/",
      SourceDirForPath(root, base::FilePath("/source/foo/bar/baz/")).value());

  // Should be case-sensitive.
  EXPECT_EQ("/SOURCE/foo/bar/",
            SourceDirForPath(root, base::FilePath("/SOURCE/foo/bar/")).value());

  // Empty source dir.
  base::FilePath empty;
  EXPECT_EQ("/source/foo/",
            SourceDirForPath(empty, base::FilePath("/source/foo")).value());
#endif
}

TEST(FilesystemUtils, ContentsEqual) {
  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());

  std::string data = "foo";

  base::FilePath file_path = temp_dir.GetPath().AppendASCII("foo.txt");
  base::WriteFile(file_path, data.c_str(), static_cast<int>(data.size()));

  EXPECT_TRUE(ContentsEqual(file_path, data));

  // Different length and contents.
  data += "bar";
  EXPECT_FALSE(ContentsEqual(file_path, data));

  // The same length, different contents.
  EXPECT_FALSE(ContentsEqual(file_path, "bar"));
}

TEST(FilesystemUtils, GetToolchainDirs) {
  BuildSettings build_settings;
  build_settings.SetBuildDir(SourceDir("//out/Debug/"));

  // The default toolchain.
  Settings default_settings(&build_settings, "");
  Label default_toolchain_label(SourceDir("//toolchain/"), "default");
  default_settings.set_toolchain_label(default_toolchain_label);
  default_settings.set_default_toolchain_label(default_toolchain_label);
  BuildDirContext default_context(&default_settings);

  // Default toolchain out dir as source dirs.
  EXPECT_EQ("//out/Debug/", GetBuildDirAsSourceDir(default_context,
                                                   BuildDirType::TOOLCHAIN_ROOT)
                                .value());
  EXPECT_EQ("//out/Debug/obj/",
            GetBuildDirAsSourceDir(default_context, BuildDirType::OBJ).value());
  EXPECT_EQ("//out/Debug/gen/",
            GetBuildDirAsSourceDir(default_context, BuildDirType::GEN).value());

  // Default toolchain our dir as output files.
  EXPECT_EQ(
      "", GetBuildDirAsOutputFile(default_context, BuildDirType::TOOLCHAIN_ROOT)
              .value());
  EXPECT_EQ(
      "obj/",
      GetBuildDirAsOutputFile(default_context, BuildDirType::OBJ).value());
  EXPECT_EQ(
      "gen/",
      GetBuildDirAsOutputFile(default_context, BuildDirType::GEN).value());

  // Check a secondary toolchain.
  Settings other_settings(&build_settings, "two/");
  Label other_toolchain_label(SourceDir("//toolchain/"), "two");
  other_settings.set_toolchain_label(other_toolchain_label);
  other_settings.set_default_toolchain_label(default_toolchain_label);
  BuildDirContext other_context(&other_settings);

  // Secondary toolchain out dir as source dirs.
  EXPECT_EQ("//out/Debug/two/",
            GetBuildDirAsSourceDir(other_context, BuildDirType::TOOLCHAIN_ROOT)
                .value());
  EXPECT_EQ("//out/Debug/two/obj/",
            GetBuildDirAsSourceDir(other_context, BuildDirType::OBJ).value());
  EXPECT_EQ("//out/Debug/two/gen/",
            GetBuildDirAsSourceDir(other_context, BuildDirType::GEN).value());

  // Secondary toolchain out dir as output files.
  EXPECT_EQ("two/",
            GetBuildDirAsOutputFile(other_context, BuildDirType::TOOLCHAIN_ROOT)
                .value());
  EXPECT_EQ("two/obj/",
            GetBuildDirAsOutputFile(other_context, BuildDirType::OBJ).value());
  EXPECT_EQ("two/gen/",
            GetBuildDirAsOutputFile(other_context, BuildDirType::GEN).value());
}

TEST(FilesystemUtils, GetSubBuildDir) {
  BuildSettings build_settings;
  build_settings.SetBuildDir(SourceDir("//out/Debug/"));

  // Test the default toolchain.
  Label default_toolchain_label(SourceDir("//toolchain/"), "default");
  Settings default_settings(&build_settings, "");
  default_settings.set_toolchain_label(default_toolchain_label);
  default_settings.set_default_toolchain_label(default_toolchain_label);
  BuildDirContext default_context(&default_settings);

  // Target in the root.
  EXPECT_EQ("//out/Debug/obj/",
            GetSubBuildDirAsSourceDir(default_context, SourceDir("//"),
                                      BuildDirType::OBJ)
                .value());
  EXPECT_EQ("gen/", GetSubBuildDirAsOutputFile(default_context, SourceDir("//"),
                                               BuildDirType::GEN)
                        .value());

  // Target in another directory.
  EXPECT_EQ("//out/Debug/obj/foo/bar/",
            GetSubBuildDirAsSourceDir(default_context, SourceDir("//foo/bar/"),
                                      BuildDirType::OBJ)
                .value());
  EXPECT_EQ("gen/foo/bar/",
            GetSubBuildDirAsOutputFile(default_context, SourceDir("//foo/bar/"),
                                       BuildDirType::GEN)
                .value());

  // Secondary toolchain.
  Settings other_settings(&build_settings, "two/");
  other_settings.set_toolchain_label(Label(SourceDir("//toolchain/"), "two"));
  other_settings.set_default_toolchain_label(default_toolchain_label);
  BuildDirContext other_context(&other_settings);

  // Target in the root.
  EXPECT_EQ("//out/Debug/two/obj/",
            GetSubBuildDirAsSourceDir(other_context, SourceDir("//"),
                                      BuildDirType::OBJ)
                .value());
  EXPECT_EQ("two/gen/", GetSubBuildDirAsOutputFile(
                            other_context, SourceDir("//"), BuildDirType::GEN)
                            .value());

  // Target in another directory.
  EXPECT_EQ("//out/Debug/two/obj/foo/bar/",
            GetSubBuildDirAsSourceDir(other_context, SourceDir("//foo/bar/"),
                                      BuildDirType::OBJ)
                .value());
  EXPECT_EQ("two/gen/foo/bar/",
            GetSubBuildDirAsOutputFile(other_context, SourceDir("//foo/bar/"),
                                       BuildDirType::GEN)
                .value());

  // Absolute source path
  EXPECT_EQ("//out/Debug/obj/ABS_PATH/abs/",
            GetSubBuildDirAsSourceDir(default_context, SourceDir("/abs"),
                                      BuildDirType::OBJ)
                .value());
  EXPECT_EQ("gen/ABS_PATH/abs/",
            GetSubBuildDirAsOutputFile(default_context, SourceDir("/abs"),
                                       BuildDirType::GEN)
                .value());
#if defined(OS_WIN)
  EXPECT_EQ("//out/Debug/obj/ABS_PATH/C/abs/",
            GetSubBuildDirAsSourceDir(default_context, SourceDir("/C:/abs"),
                                      BuildDirType::OBJ)
                .value());
  EXPECT_EQ("gen/ABS_PATH/C/abs/",
            GetSubBuildDirAsOutputFile(default_context, SourceDir("/C:/abs"),
                                       BuildDirType::GEN)
                .value());
#endif
}

TEST(FilesystemUtils, GetBuildDirForTarget) {
  BuildSettings build_settings;
  build_settings.SetBuildDir(SourceDir("//out/Debug/"));
  Settings settings(&build_settings, "");

  Target a(&settings, Label(SourceDir("//foo/bar/"), "baz"));
  EXPECT_EQ("//out/Debug/obj/foo/bar/",
            GetBuildDirForTargetAsSourceDir(&a, BuildDirType::OBJ).value());
  EXPECT_EQ("obj/foo/bar/",
            GetBuildDirForTargetAsOutputFile(&a, BuildDirType::OBJ).value());
  EXPECT_EQ("//out/Debug/gen/foo/bar/",
            GetBuildDirForTargetAsSourceDir(&a, BuildDirType::GEN).value());
  EXPECT_EQ("gen/foo/bar/",
            GetBuildDirForTargetAsOutputFile(&a, BuildDirType::GEN).value());
}

// Tests handling of output dirs when build dir is the same as the root.
TEST(FilesystemUtils, GetDirForEmptyBuildDir) {
  BuildSettings build_settings;
  build_settings.SetBuildDir(SourceDir("//"));
  Settings settings(&build_settings, "");

  BuildDirContext context(&settings);

  EXPECT_EQ(
      "//",
      GetBuildDirAsSourceDir(context, BuildDirType::TOOLCHAIN_ROOT).value());
  EXPECT_EQ("//gen/",
            GetBuildDirAsSourceDir(context, BuildDirType::GEN).value());
  EXPECT_EQ("//obj/",
            GetBuildDirAsSourceDir(context, BuildDirType::OBJ).value());

  EXPECT_EQ(
      "",
      GetBuildDirAsOutputFile(context, BuildDirType::TOOLCHAIN_ROOT).value());
  EXPECT_EQ("gen/",
            GetBuildDirAsOutputFile(context, BuildDirType::GEN).value());
  EXPECT_EQ("obj/",
            GetBuildDirAsOutputFile(context, BuildDirType::OBJ).value());
}

TEST(FilesystemUtils, ResolveRelativeTest) {
  std::string result;
#ifndef OS_WIN
  EXPECT_TRUE(
      MakeAbsolutePathRelativeIfPossible("/some/dir", "/some/dir/a", &result));
  EXPECT_EQ(result, "//a");

  EXPECT_FALSE(MakeAbsolutePathRelativeIfPossible(
      "/some/dir", "/some/dir-sufix/a", &result));
#else
  EXPECT_TRUE(MakeAbsolutePathRelativeIfPossible("C:/some/dir",
                                                 "/C:/some/dir/a", &result));
  EXPECT_EQ(result, "//a");
  EXPECT_FALSE(MakeAbsolutePathRelativeIfPossible(
      "C:/some/dir", "C:/some/dir-sufix/a", &result));
#endif
}
