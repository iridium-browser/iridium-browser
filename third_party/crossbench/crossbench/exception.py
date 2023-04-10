# Copyright 2022 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

from dataclasses import dataclass
import logging
import sys
import traceback as tb
from types import TracebackType
from typing import Dict, List, Optional, Tuple, Type

from crossbench import helper

TInfoStack = Tuple[str, ...]

TExceptionTypes = Tuple[Type[BaseException], ...]


@dataclass
class Entry:
  traceback: str
  exception: BaseException
  info_stack: TInfoStack


class MultiException(ValueError):
  """Default exception thrown by ExceptionAnnotator.assert_success.
  It holds on to the ExceptionAnnotator and its previously captured exceptions
  are automatically added to active ExceptionAnnotator in an
  ExceptionAnnotationScope."""

  def __init__(self, message: str, exceptions: ExceptionAnnotator):
    super().__init__(message)
    self.exceptions = exceptions


class ExceptionAnnotationScope:
  """Used in a with-scope to annotate exceptions with a TInfoStack.

  Used via the capture/annotate/info helper methods on
  ExceptionAnnotator.
  """

  def __init__(self,
               annotator: ExceptionAnnotator,
               exception_types: TExceptionTypes,
               entries: Tuple[str, ...],
               rethrow: bool = False):
    logging.debug("ExceptionAnnotationScope: %s", entries)
    self._annotator = annotator
    self._exception_types = exception_types
    self._added_info_stack_entries = entries
    self.rethrow = rethrow
    self._previous_info_stack: TInfoStack = ()

  def __enter__(self):
    self._annotator._pending_exceptions.clear()
    self._previous_info_stack = self._annotator.info_stack
    self._annotator._info_stack = self._previous_info_stack + (
        self._added_info_stack_entries)

  def __exit__(self, exception_type: Optional[Type[BaseException]],
               exception_value: Optional[BaseException],
               traceback: Optional[TracebackType]) -> bool:
    if not exception_value:
      self._annotator._info_stack = self._previous_info_stack
      # False => exception not handled
      return False
    logging.debug("Intermediate Exception: %s:%s", exception_type,
                  exception_value)
    if self._exception_types and exception_type and (
        issubclass(exception_type, MultiException) or
        issubclass(exception_type, self._exception_types)):
      # Handle matching exceptions directly here and prevent further
      # exception handling by returning True.
      self._annotator.append(exception_value)
      self._annotator._info_stack = self._previous_info_stack
      if self.rethrow:
        self._annotator.assert_success(log=False)
      return True
    if exception_value not in self._annotator._pending_exceptions:
      self._annotator._pending_exceptions[
          exception_value] = self._annotator.info_stack
    # False => exception not handled
    return False


class ExceptionAnnotator:
  """Collects exceptions with full backtraces and user-provided info stacks.

  Additional stack information is constructed from active
  ExceptionAnnotationScopes.
  """

  def __init__(self, throw: bool = False):
    self._exceptions: List[Entry] = []
    self.throw: bool = throw
    # The info_stack adds additional meta information to handle exceptions.
    # Unlike the source-based backtrace, this can contain dynamic information
    # for easier debugging.
    self._info_stack: TInfoStack = ()
    # Associates raised exception with the info_stack at that time for later
    # use in the `handle` method.
    # This is cleared whenever we enter a  new ExceptionAnnotationScope.
    self._pending_exceptions: Dict[BaseException, TInfoStack] = {}

  @property
  def is_success(self) -> bool:
    return len(self._exceptions) == 0

  @property
  def info_stack(self) -> TInfoStack:
    return self._info_stack

  @property
  def exceptions(self) -> List[Entry]:
    return self._exceptions

  def assert_success(self,
                     message: Optional[str] = None,
                     exception_cls: Type[BaseException] = MultiException,
                     log: bool = True) -> None:
    if self.is_success:
      return
    if log:
      self.log()
    if message is None:
      message = "Got Exceptions: {}"
    message = message.format(self)
    if issubclass(exception_cls, MultiException):
      raise exception_cls(message, self)
    raise exception_cls(message)

  def info(self, *stack_entries: str) -> ExceptionAnnotationScope:
    """Only sets info stack entries, exceptions are passed-through."""
    return ExceptionAnnotationScope(self, tuple(), stack_entries)

  def capture(self,
              *stack_entries: str,
              exceptions: TExceptionTypes = (Exception,),
              rethrow: bool = False) -> ExceptionAnnotationScope:
    """Sets info stack entries and captures exceptions."""
    return ExceptionAnnotationScope(self, exceptions, stack_entries, rethrow)

  def extend(self, annotator: ExceptionAnnotator,
             is_nested: bool = False) -> None:
    if is_nested:
      self._extend_with_prepended_stack_info(annotator)
    else:
      self._exceptions.extend(annotator.exceptions)

  def _extend_with_prepended_stack_info(self,
                                        annotator: ExceptionAnnotator) -> None:
    for entry in annotator.exceptions:
      merged_info_stack = self.info_stack + entry.info_stack
      merged_entry = Entry(entry.traceback, entry.exception, merged_info_stack)
      self._exceptions.append(merged_entry)

  def append(self, exception: BaseException) -> None:
    traceback: str = tb.format_exc()
    logging.debug("Intermediate Exception %s:%s", type(exception), exception)
    logging.debug(traceback)
    if isinstance(exception, KeyboardInterrupt):
      # Fast exit on KeyboardInterrupts for a better user experience.
      sys.exit(0)
    if isinstance(exception, MultiException):
      # Directly add exceptions from nested annotators.
      self.extend(exception.exceptions, is_nested=True)
    else:
      stack = self.info_stack
      if exception in self._pending_exceptions:
        stack = self._pending_exceptions[exception]
      self._exceptions.append(Entry(traceback, exception, stack))
    if self.throw:
      raise  # pylint: disable=misplaced-bare-raise

  def log(self) -> None:
    if self.is_success:
      return
    logging.error("=" * 80)
    logging.error("ERRORS occurred:")
    logging.error("=" * 80)
    for entry in self._exceptions:
      logging.debug(entry.exception)
      logging.debug(entry.traceback)
      logging.debug("-" * 80)
    for info_stack, entries in helper.group_by(
        self._exceptions, key=lambda entry: tuple(entry.info_stack)).items():
      if info_stack:
        info = "Info: "
        joiner = "\n" + (" " * (len(info) - 2)) + "> "
        message = f"{info}{joiner.join(info_stack)}"
        logging.error(message)
      for entry in entries:
        logging.error("- " * 40)
        logging.error("Type: %s:", entry.exception.__class__.__name__)
        logging.error("      %s", entry.exception)
      logging.error("-" * 80)

  def to_json(self) -> list:
    return [{
        "title": str(entry.exception),
        "trace": str(entry.traceback).splitlines(),
        "info_stack": entry.info_stack
    } for entry in self._exceptions]

  def __str__(self) -> str:
    return "\n".join(str(entry.exception) for entry in self._exceptions)

# Expose simpler name
Annotator = ExceptionAnnotator


def annotate(*stack_entries: str,
             exceptions: TExceptionTypes = (Exception,),
             throw: bool = False) -> ExceptionAnnotationScope:
  return ExceptionAnnotator(throw=throw).capture(
      *stack_entries, exceptions=exceptions, rethrow=True)
