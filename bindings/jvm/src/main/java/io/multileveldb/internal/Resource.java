package io.multileveldb.internal;

import com.sun.jna.Pointer;
import io.multileveldb.errors.ClosedException;

import java.lang.ref.PhantomReference;
import java.lang.ref.ReferenceQueue;
import java.util.Set;
import java.util.concurrent.ConcurrentHashMap;
import java.util.function.Consumer;
import java.util.function.Function;

/**
 * An owned native resource with deterministic, idempotent close.
 * Methods synchronize against cleanup. Phantom-reference cleanup is a fallback;
 * release promptly with try-with-resources to avoid retaining file locks.
 *
 * @hidden
 */
public abstract class Resource implements AutoCloseable {
    private static final ReferenceQueue<Resource> QUEUE = new ReferenceQueue<>();
    private static final Set<Cleanup> LIVE = ConcurrentHashMap.newKeySet();

    static {
        Thread cleaner = new Thread(() -> {
            while (true) {
                try {
                    ((Cleanup) QUEUE.remove()).clean();
                } catch (InterruptedException ignored) { /* Cleanup lives for the process lifetime. */ }
            }
        }, "multileveldb-cleanup");
        cleaner.setDaemon(true);
        cleaner.setContextClassLoader(null);
        cleaner.start();
    }

    private final State state;
    private final Cleanup cleanup;

    protected Resource(Pointer p, Consumer<Pointer> free) {
        state = new State(p, free);
        cleanup = new Cleanup(this, state);
    }

    protected final <T> T use(Function<Pointer, T> fn) {
        return state.use(fn);
    }

    /**
     * Release this wrapper's native handle once; repeated calls do nothing.
     * Subsequent resource operations throw {@link ClosedException}.
     * A retained iterator may keep its engine/snapshot alive independently.
     */
    @Override
    public final void close() {
        cleanup.clean();
    }

    private static final class Cleanup extends PhantomReference<Resource> {
        private final State state;

        Cleanup(Resource owner, State state) {
            super(owner, QUEUE);
            this.state = state;
            LIVE.add(this);
        }

        void clean() {
            try {
                state.run();
            } finally {
                clear();
                LIVE.remove(this);
            }
        }
    }

    private static final class State implements Runnable {
        final Consumer<Pointer> free;
        Pointer pointer;

        State(Pointer p, Consumer<Pointer> f) {
            pointer = p;
            free = f;
        }

        public synchronized void run() {
            if (pointer != null) {
                free.accept(pointer);
                pointer = null;
            }
        }

        synchronized <T> T use(Function<Pointer, T> fn) {
            if (pointer == null) throw new ClosedException("Resource is closed");
            return fn.apply(pointer);
        }
    }
}
