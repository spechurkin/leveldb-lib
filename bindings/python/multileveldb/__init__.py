"""Synchronous binary LevelDB API. Set MULTILEVELDB_LIBRARY before first use.

Keys/values accept bytes, bytearray or memoryview and return independent bytes.
Missing get returns None; an existing empty value returns b''. All resources
support close() and context managers. Calls and cleanup share one reentrant lock.
See README.md for options, native errors and cursor/snapshot lifetime rules.
"""
from .batch import Batch
from .cursor import Cursor
from .database import Database
from .errors import Error
from .snapshot import Snapshot

__all__ = ["Database", "Batch", "Snapshot", "Cursor", "Error"]
