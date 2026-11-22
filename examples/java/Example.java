import io.multileveldb.Database;
import io.multileveldb.options.OpenOptions;

import java.nio.file.Paths;
import java.util.Arrays;

public final class Example {
    public static void main(String[] args) {
        try (OpenOptions options = new OpenOptions().createIfMissing(true);
             Database db = Database.open(Paths.get("example-db"), options)) {
            byte[] key = {0, -1}, value = {0, -128, -1};
            db.put(key, value);
            if (!Arrays.equals(db.get(key), value)) throw new AssertionError();
        }
    }
}
