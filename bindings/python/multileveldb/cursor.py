from ._native import _bytes, _call, _lib, _lock, _read
from ._resource import _Resource


class Cursor(_Resource):
    """Owned bidirectional cursor created by Database.iterator()."""

    def seek_to_first(self):
        """Position at the first record; empty views become invalid."""
        with _lock:
            _call("iterator_seek_first", self._handle())

    def seek_to_last(self):
        """Position at the last record; empty views become invalid."""
        with _lock:
            _call("iterator_seek_last", self._handle())

    def seek(self, key):
        """Position at the first key >= target, or an invalid end position."""
        key = _bytes(key)
        with _lock:
            _call("iterator_seek", self._handle(), key, len(key))

    def next(self):
        """Advance; passing the end succeeds, an already invalid cursor fails."""
        with _lock:
            _call("iterator_next", self._handle())

    def previous(self):
        """Move backward; an already invalid cursor raises NOT_FOUND."""
        with _lock:
            _call("iterator_previous", self._handle())

    def is_valid(self):
        """Test position; call status() separately to detect scan errors."""
        with _lock:
            return bool(_lib().ml_iterator_valid(self._handle()))

    def key(self):
        """Return a copied key; invalid position raises NOT_FOUND."""
        with _lock:
            return _read("iterator_key", self._handle())

    def value(self):
        """Return a copied value, including b''; invalid position raises NOT_FOUND."""
        with _lock:
            return _read("iterator_value", self._handle())

    def status(self):
        """Raise on recorded scan failure; ordinary exhaustion succeeds."""
        with _lock:
            _call("iterator_status", self._handle())
