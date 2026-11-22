package io.multileveldb;

import com.sun.jna.Pointer;
import io.multileveldb.internal.Resource;

import static io.multileveldb.internal.NativeCalls.N;

/**
 * Immutable engine read view; pass to get/iterator on its originating DB.
 * Closing releases this wrapper's reference; retained cursors keep theirs.
 * Obtain a snapshot with {@link Database#createSnapshot()} and close it with
 * try-with-resources.
 */
public final class Snapshot extends Resource {
    Snapshot(Pointer p) {
        super(p, N::ml_snapshot_release);
    }
}
