import os

from ._native import _bytes, _call, _create, _lock, _read
from ._options import _options
from ._resource import _Resource
from .batch import Batch
from .cursor import Cursor
from .errors import Error
from .snapshot import Snapshot, _snapshot


class Database(_Resource):
    """Owned database directory. Construct with open() or create(), close promptly."""

    @classmethod
    def create(cls, path, **open_options):
        """Create a new database and return its open handle; parents must exist.

        Copies storage settings, forcing create_if_missing and error_if_exists
        true. Existing databases raise Error with INVALID_ARGUMENT, or
        DATABASE_LOCKED if locked. Other path/native errors follow open().
        Requires native ABI 1.1 or later.
        """
        path = os.fspath(path)
        if not isinstance(path, str):
            raise TypeError("Path must be str or a text PathLike")
        encoded = path.encode("utf-8", "strict")
        with _lock, _options(0, open_options) as options:
            return cls(_create("db_create", encoded, len(encoded), options._handle()), "db_destroy")

    @classmethod
    def open(cls, path, **open_options):
        """Open a UTF-8 str/PathLike path; creation defaults to disabled."""
        path = os.fspath(path)
        if not isinstance(path, str):
            raise TypeError("Path must be str or a text PathLike")
        encoded = path.encode("utf-8", "strict")
        with _lock, _options(0, open_options) as options:
            return cls(_create("db_open", encoded, len(encoded), options._handle()), "db_destroy")

    def get(self, key, read_options=None, snapshot=None):
        """Return copied bytes or None if missing; snapshot must belong to this DB."""
        key = _bytes(key)
        with _lock, _options(1, read_options) as options:
            try:
                return _read("db_get", self._handle(), key, len(key), options._handle(), _snapshot(snapshot))
            except Error as error:
                if error.status == 1:
                    return None
                raise

    def put(self, key, value, write_options=None):
        """Insert/replace binary data; optional {'sync': True} requests flush."""
        key, value = _bytes(key), _bytes(value)
        with _lock, _options(2, write_options) as options:
            _call("db_put", self._handle(), key, len(key), value, len(value), options._handle())

    def delete(self, key, write_options=None):
        """Delete a key; deleting an absent key succeeds."""
        key = _bytes(key)
        with _lock, _options(2, write_options) as options:
            _call("db_delete", self._handle(), key, len(key), options._handle())

    def write(self, batch, write_options=None):
        """Apply a Batch atomically in order without clearing/consuming it."""
        if not isinstance(batch, Batch):
            raise TypeError("Expected Batch")
        with _lock, _options(2, write_options) as options:
            _call("batch_write", self._handle(), batch._handle(), options._handle())

    def create_snapshot(self):
        """Capture a native read view retaining this engine and its lock."""
        with _lock:
            return Snapshot(_create("snapshot_create", self._handle()), "snapshot_release")

    def iterator(self, read_options=None, snapshot=None):
        """Return an unpositioned streaming cursor retaining its engine/view."""
        with _lock, _options(1, read_options) as options:
            return Cursor(_create("iterator_create", self._handle(), options._handle(), _snapshot(snapshot)),
                          "iterator_destroy")
