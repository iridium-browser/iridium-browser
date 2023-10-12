#!/usr/bin/env vpython3
"""Tests for split_cl."""

import os
import sys
import unittest
from unittest import mock

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

import split_cl


class SplitClTest(unittest.TestCase):
  def testAddUploadedByGitClSplitToDescription(self):
    description = """Convert use of X to Y in $directory

<add some background about this conversion for the reviewers>

"""
    footers = 'Bug: 12345'

    added_line = 'This CL was uploaded by git cl split.'

    # Description without footers
    self.assertEqual(split_cl.AddUploadedByGitClSplitToDescription(description),
                     description + added_line)
    # Description with footers
    self.assertEqual(
        split_cl.AddUploadedByGitClSplitToDescription(description + footers),
        description + added_line + '\n\n' + footers)

  def testFormatDescriptionOrComment(self):
    description = "Converted use of X to Y in $directory."

    # One directory
    self.assertEqual(split_cl.FormatDescriptionOrComment(description, ["foo"]),
                     "Converted use of X to Y in /foo.")

    # Many directories
    self.assertEqual(
        split_cl.FormatDescriptionOrComment(description, ["foo", "bar"]),
        "Converted use of X to Y in ['/foo', '/bar'].")

  def GetDirectoryBaseName(self, file_path):
    return os.path.basename(os.path.dirname(file_path))

  def MockSuggestOwners(self, paths, exclude=None):
    if not paths:
      return ["superowner"]
    return self.GetDirectoryBaseName(paths[0]).split(",")

  def MockIsFile(self, file_path):
    if os.path.basename(file_path) == "OWNERS":
      return "owner" in self.GetDirectoryBaseName(file_path)

    return True

  @mock.patch("os.path.isfile")
  def testSelectReviewersForFiles(self, mock_is_file):
    mock_is_file.side_effect = self.MockIsFile

    owners_client = mock.Mock(SuggestOwners=self.MockSuggestOwners,
                              EVERYONE="*")
    cl = mock.Mock(owners_client=owners_client)

    files = [("M", os.path.join("foo", "owner1,owner2", "a.txt")),
             ("M", os.path.join("foo", "owner1,owner2", "b.txt")),
             ("M", os.path.join("bar", "owner1,owner2", "c.txt")),
             ("M", os.path.join("bax", "owner2", "d.txt")),
             ("M", os.path.join("baz", "owner3", "e.txt"))]

    files_split_by_reviewers = split_cl.SelectReviewersForFiles(
        cl, "author", files, 0)

    self.assertEqual(3, len(files_split_by_reviewers.keys()))
    info1 = files_split_by_reviewers[tuple(["owner1", "owner2"])]
    self.assertEqual(info1.files,
                     [("M", os.path.join("foo", "owner1,owner2", "a.txt")),
                      ("M", os.path.join("foo", "owner1,owner2", "b.txt")),
                      ("M", os.path.join("bar", "owner1,owner2", "c.txt"))])
    self.assertEqual(info1.owners_directories,
                     ["foo/owner1,owner2", "bar/owner1,owner2"])
    info2 = files_split_by_reviewers[tuple(["owner2"])]
    self.assertEqual(info2.files,
                     [("M", os.path.join("bax", "owner2", "d.txt"))])
    self.assertEqual(info2.owners_directories, ["bax/owner2"])
    info3 = files_split_by_reviewers[tuple(["owner3"])]
    self.assertEqual(info3.files,
                     [("M", os.path.join("baz", "owner3", "e.txt"))])
    self.assertEqual(info3.owners_directories, ["baz/owner3"])

  class UploadClTester:
    """Sets up test environment for testing split_cl.UploadCl()"""
    def __init__(self, test):
      self.mock_git_branches = self.StartPatcher("git_common.branches", test)
      self.mock_git_branches.return_value = []
      self.mock_git_current_branch = self.StartPatcher(
          "git_common.current_branch", test)
      self.mock_git_current_branch.return_value = "branch_to_upload"
      self.mock_git_run = self.StartPatcher("git_common.run", test)
      self.mock_temporary_file = self.StartPatcher(
          "gclient_utils.temporary_file", test)
      self.mock_temporary_file().__enter__.return_value = "temporary_file0"
      self.mock_file_writer = self.StartPatcher("gclient_utils.FileWrite", test)

    def StartPatcher(self, target, test):
      patcher = mock.patch(target)
      test.addCleanup(patcher.stop)
      return patcher.start()

    def DoUploadCl(self, directories, files, reviewers, cmd_upload):
      split_cl.UploadCl("branch_to_upload", "upstream_branch",
                        directories, files, "description", None, reviewers,
                        mock.Mock(), cmd_upload, True, True, "topic",
                        os.path.sep)

  def testUploadCl(self):
    """Tests commands run by UploadCl."""

    upload_cl_tester = self.UploadClTester(self)

    directories = ["dir0"]
    files = [("M", os.path.join("bar", "a.cc")),
             ("D", os.path.join("foo", "b.cc"))]
    reviewers = {"reviewer1@gmail.com", "reviewer2@gmail.com"}
    mock_cmd_upload = mock.Mock()
    upload_cl_tester.DoUploadCl(directories, files, reviewers, mock_cmd_upload)

    abs_repository_path = os.path.abspath(os.path.sep)
    mock_git_run = upload_cl_tester.mock_git_run
    self.assertEqual(mock_git_run.call_count, 4)
    mock_git_run.assert_has_calls([
        mock.call("checkout", "-t", "upstream_branch", "-b",
                  "branch_to_upload_dir0_split"),
        mock.call("rm", os.path.join(abs_repository_path, "foo", "b.cc")),
        mock.call("checkout", "branch_to_upload", "--",
                  os.path.join(abs_repository_path, "bar", "a.cc")),
        mock.call("commit", "-F", "temporary_file0")
    ])

    expected_upload_args = [
        "-f", "-r", "reviewer1@gmail.com,reviewer2@gmail.com", "--cq-dry-run",
        "--send-mail", "--enable-auto-submit", "--topic=topic"
    ]
    mock_cmd_upload.assert_called_once_with(expected_upload_args)

  def testDontUploadClIfBranchAlreadyExists(self):
    """Tests that a CL is not uploaded if split branch already exists"""

    upload_cl_tester = self.UploadClTester(self)
    upload_cl_tester.mock_git_branches.return_value = [
        "branch0", "branch_to_upload_dir0_split"
    ]

    directories = ["dir0"]
    files = [("M", os.path.join("bar", "a.cc")),
             ("D", os.path.join("foo", "b.cc"))]
    reviewers = {"reviewer1@gmail.com"}
    mock_cmd_upload = mock.Mock()
    upload_cl_tester.DoUploadCl(directories, files, reviewers, mock_cmd_upload)

    upload_cl_tester.mock_git_run.assert_not_called()
    mock_cmd_upload.assert_not_called()

  @mock.patch("gclient_utils.AskForData")
  def testCheckDescriptionBugLink(self, mock_ask_for_data):
    # Description contains bug link.
    self.assertTrue(split_cl.CheckDescriptionBugLink("Bug:1234"))
    self.assertEqual(mock_ask_for_data.call_count, 0)

    # Description does not contain bug link. User does not enter 'y' when
    # prompted.
    mock_ask_for_data.reset_mock()
    mock_ask_for_data.return_value = "m"
    self.assertFalse(split_cl.CheckDescriptionBugLink("Description"))
    self.assertEqual(mock_ask_for_data.call_count, 1)

    # Description does not contain bug link. User enters 'y' when prompted.
    mock_ask_for_data.reset_mock()
    mock_ask_for_data.return_value = "y"
    self.assertTrue(split_cl.CheckDescriptionBugLink("Description"))
    self.assertEqual(mock_ask_for_data.call_count, 1)


if __name__ == '__main__':
  unittest.main()
