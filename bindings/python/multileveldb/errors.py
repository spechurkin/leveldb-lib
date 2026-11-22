_codes = ("OK", "NOT_FOUND", "INVALID_ARGUMENT", "IO_ERROR", "CORRUPTION",
          "DATABASE_LOCKED", "DATABASE_CLOSED", "UNSUPPORTED", "INTERNAL_ERROR")


class Error(Exception):
    """Native failure with numeric status, stable code and original message."""

    def __init__(self, status, message):
        self.status = status
        self.code = _codes[status] if 0 <= status < len(_codes) else "INTERNAL_ERROR"
        self.message = message
        super().__init__(f"{self.code}: {message}")
