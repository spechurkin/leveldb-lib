from ._native import _call, _create
from ._resource import _Resource

_fields = {
    "create_if_missing": (0, 1), "error_if_exists": (0, 2), "paranoid_checks": (0, 3),
    "write_buffer_size": (0, 4), "max_open_files": (0, 5), "block_size": (0, 6),
    "block_restart_interval": (0, 7), "compression": (0, 8), "cache_size": (0, 9),
    "verify_checksums": (1, 10), "fill_cache": (1, 11), "sync": (2, 12),
}


def _options(kind, values):
    resource = _Resource(_create("options_create", kind), "options_destroy")
    try:
        if values is not None:
            if not isinstance(values, dict):
                raise TypeError("Options must be a dict or None")
            for name, value in values.items():
                if name not in _fields or _fields[name][0] != kind:
                    raise ValueError(f"Invalid option for this operation: {name}")
                if name == "compression":
                    if value not in ("none", "snappy"):
                        raise ValueError("compression must be none or snappy")
                    value = int(value == "snappy")
                if not isinstance(value, int) or not 0 <= value <= 0xffffffffffffffff:
                    raise ValueError(f"{name} must be a uint64 integer")
                _call("options_set", resource._handle(), _fields[name][1], value)
        return resource
    except BaseException:
        resource.close()
        raise
