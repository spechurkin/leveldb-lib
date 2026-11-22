import io.multileveldb.Database
import io.multileveldb.options.OpenOptions
import java.nio.file.Paths

fun main() {
    OpenOptions().createIfMissing(true).use { options ->
        Database.open(Paths.get("example-db"), options).use { db ->
            val key = byteArrayOf(0, -1)
            val value = byteArrayOf(0, -128, -1)
            db.put(key, value)
            check(db.get(key).contentEquals(value))
        }
    }
}
