from ._resource import _Resource


class Snapshot(_Resource):
    """Immutable same-instance read view created by Database.create_snapshot()."""


def _snapshot(snapshot):
    if snapshot is not None and not isinstance(snapshot, Snapshot):
        raise TypeError("Expected Snapshot or None")
    return snapshot._handle() if snapshot is not None else None
