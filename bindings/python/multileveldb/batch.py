from ._native import _bytes, _call, _create, _lock
from ._resource import _Resource


class Batch(_Resource):
    """Database-independent, reusable WriteBatch. Use with or close()."""

    def __init__(self):
        """Allocate an empty native batch; nothing is written yet."""
        with _lock:
            super().__init__(_create("batch_create"), "batch_destroy")

    def put(self, key, value):
        """Append copied binary data and return this batch for chaining."""
        key, value = _bytes(key), _bytes(value)
        with _lock:
            _call("batch_put", self._handle(), key, len(key), value, len(value))
        return self

    def delete(self, key):
        """Append a copied deletion and return this batch."""
        key = _bytes(key)
        with _lock:
            _call("batch_delete", self._handle(), key, len(key))
        return self

    def clear(self):
        """Discard queued operations, keeping the batch reusable."""
        with _lock:
            _call("batch_clear", self._handle())
