package io.multileveldb.options;

import io.multileveldb.errors.ClosedException;
import io.multileveldb.errors.InvalidArgumentException;
import io.multileveldb.internal.Resource;

import static io.multileveldb.internal.NativeCalls.*;

/**
 * Owned base for kind-specific engine settings; not directly constructible.
 * Database calls copy settings, so options may close after the call returns.
 * Setters after close throw {@link ClosedException}.
 */
public abstract class Options extends Resource {
    Options(int kind) {
        super(create((p, e) -> N.ml_options_create(kind, p, e)), N::ml_options_destroy);
    }

    final void set(int field, long value) {
        if (value < 0) throw new InvalidArgumentException("Option cannot be negative");
        use(p -> {
            call(e -> N.ml_options_set(p, field, value, e));
            return null;
        });
    }
}
