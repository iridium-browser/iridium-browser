# Copyright (c) 2020 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import itertools
import os
import random
import threading

import gerrit_util
import git_common
import owners as owners_db
import scm


class OwnersClient(object):
  """Interact with OWNERS files in a repository.

  This class allows you to interact with OWNERS files in a repository both the
  Gerrit Code-Owners plugin REST API, and the owners database implemented by
  Depot Tools in owners.py:

   - List all the owners for a group of files.
   - Check if files have been approved.
   - Suggest owners for a group of files.

  All code should use this class to interact with OWNERS files instead of the
  owners database in owners.py
  """
  # '*' means that everyone can approve.
  EVERYONE = '*'

  # Possible status of a file.
  # - INSUFFICIENT_REVIEWERS: The path needs owners approval, but none of its
  #   owners is currently a reviewer of the change.
  # - PENDING: An owner of this path has been added as reviewer, but approval
  #   has not been given yet.
  # - APPROVED: The path has been approved by an owner.
  APPROVED = 'APPROVED'
  PENDING = 'PENDING'
  INSUFFICIENT_REVIEWERS = 'INSUFFICIENT_REVIEWERS'

  def ListOwners(self, path):
    """List all owners for a file.

    The returned list is sorted so that better owners appear first.
    """
    raise Exception('Not implemented')

  def BatchListOwners(self, paths):
    """List all owners for a group of files.

    Returns a dictionary {path: [owners]}.
    """
    with git_common.ScopedPool(kind='threads') as pool:
      return dict(pool.imap_unordered(
          lambda p: (p, self.ListOwners(p)), paths))

  def GetFilesApprovalStatus(self, paths, approvers, reviewers):
    """Check the approval status for the given paths.

    Utility method to check for approval status when a change has not yet been
    created, given reviewers and approvers.

    See GetChangeApprovalStatus for description of the returned value.
    """
    approvers = set(approvers)
    if approvers:
      approvers.add(self.EVERYONE)
    reviewers = set(reviewers)
    if reviewers:
      reviewers.add(self.EVERYONE)
    status = {}
    owners_by_path = self.BatchListOwners(paths)
    for path, owners in owners_by_path.items():
      owners = set(owners)
      if owners.intersection(approvers):
        status[path] = self.APPROVED
      elif owners.intersection(reviewers):
        status[path] = self.PENDING
      else:
        status[path] = self.INSUFFICIENT_REVIEWERS
    return status

  def ScoreOwners(self, paths, exclude=None):
    """Get sorted list of owners for the given paths."""
    if not paths:
      return []
    exclude = exclude or []
    owners = []
    queues = self.BatchListOwners(paths).values()
    for i in range(max(len(q) for q in queues)):
      for q in queues:
        if i < len(q) and q[i] not in owners and q[i] not in exclude:
          owners.append(q[i])
    return owners

  def SuggestOwners(self, paths, exclude=None):
    """Suggest a set of owners for the given paths."""
    exclude = exclude or []

    paths_by_owner = {}
    owners_by_path = self.BatchListOwners(paths)
    for path, owners in owners_by_path.items():
      for owner in owners:
        paths_by_owner.setdefault(owner, set()).add(path)

    selected = []
    missing = set(paths)
    for owner in self.ScoreOwners(paths, exclude=exclude):
      missing_len = len(missing)
      missing.difference_update(paths_by_owner[owner])
      if missing_len > len(missing):
        selected.append(owner)
      if not missing:
        break

    return selected


class DepotToolsClient(OwnersClient):
  """Implement OwnersClient using owners.py Database."""
  def __init__(self, root, branch, fopen=open, os_path=os.path):
    super(DepotToolsClient, self).__init__()

    self._root = root
    self._branch = branch
    self._fopen = fopen
    self._os_path = os_path
    self._db = None
    self._db_lock = threading.Lock()

  def _ensure_db(self):
    if self._db is not None:
      return
    self._db = owners_db.Database(self._root, self._fopen, self._os_path)
    self._db.override_files = self._GetOriginalOwnersFiles()

  def _GetOriginalOwnersFiles(self):
    return {
      f: scm.GIT.GetOldContents(self._root, f, self._branch).splitlines()
      for _, f in scm.GIT.CaptureStatus(self._root, self._branch)
      if os.path.basename(f) == 'OWNERS'
    }

  def ListOwners(self, path):
    # all_possible_owners is not thread safe.
    with self._db_lock:
      self._ensure_db()
      # all_possible_owners returns a dict {owner: [(path, distance)]}. We want
      # to return a list of owners sorted by increasing distance.
      distance_by_owner = self._db.all_possible_owners([path], None)
      # We add a small random number to the distance, so that owners at the
      # same distance are returned in random order to avoid overloading those
      # who would appear first.
      return sorted(
          distance_by_owner,
          key=lambda o: distance_by_owner[o][0][1] + random.random())


class GerritClient(OwnersClient):
  """Implement OwnersClient using OWNERS REST API."""
  def __init__(self, host, project, branch):
    super(GerritClient, self).__init__()

    self._host = host
    self._project = project
    self._branch = branch
    self._owners_cache = {}

    # Seed used by Gerrit to shuffle code owners that have the same score. Can
    # be used to make the sort order stable across several requests, e.g. to get
    # the same set of random code owners for different file paths that have the
    # same code owners.
    self._seed = random.getrandbits(30)

  def ListOwners(self, path):
    # Always use slashes as separators.
    path = path.replace(os.sep, '/')
    if path not in self._owners_cache:
      # GetOwnersForFile returns a list of account details sorted by order of
      # best reviewer for path. If owners have the same score, the order is
      # random, seeded by `self._seed`.
      data = gerrit_util.GetOwnersForFile(
          self._host, self._project, self._branch, path,
          resolve_all_users=False, seed=self._seed)
      self._owners_cache[path] = [
        d['account']['email']
        for d in data['code_owners']
        if 'account' in d and 'email' in d['account']
      ]
      # If owned_by_all_users is true, add everyone as an owner at the end of
      # the owners list.
      if data.get('owned_by_all_users', False):
        self._owners_cache[path].append(self.EVERYONE)
    return self._owners_cache[path]


def GetCodeOwnersClient(root, upstream, host, project, branch):
  """Get a new OwnersClient.

  Defaults to GerritClient, and falls back to DepotToolsClient if code-owners
  plugin is not available."""
  if gerrit_util.IsCodeOwnersEnabledOnHost(host):
    return GerritClient(host, project, branch)
  return DepotToolsClient(root, upstream)
