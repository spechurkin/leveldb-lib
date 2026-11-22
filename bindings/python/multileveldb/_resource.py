import weakref

from ._native import _lib, _lock
from .errors import Error


def _release(lib, free, pointer):
    with _lock:
        getattr(lib, "ml_" + free)(pointer)


class _Resource:
    def __init__(self, pointer, free):
        self._pointer = pointer
        self._cleanup = weakref.finalize(self, _release, _lib(), free, pointer)

    def _handle(self):
        if not self._cleanup.alive:
            raise Error(6, "Resource is closed")
        return self._pointer

    def close(self):
        """Release once; dependents retain their own native engine references."""
        with _lock:
            self._cleanup()

    def __enter__(self):
        """Return this live resource for a with block."""
        with _lock:
            self._handle()
        return self

    def __exit__(self, *_):
        """Close even if the with block raises; do not suppress exceptions."""
        self.close()
