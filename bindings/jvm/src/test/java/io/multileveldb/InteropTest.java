package io.multileveldb;


import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.Arrays;

public final class InteropTest {
    static byte[] unhex(String s) {
        if (s.equals("-")) return new byte[0];
        byte[] bytes = new byte[s.length() / 2];
        for (int i = 0; i < bytes.length; i++) bytes[i] = (byte) Integer.parseInt(s.substring(i * 2, i * 2 + 2), 16);
        return bytes;
    }

    public static void main(String[] args) throws Exception {
        try (Database db = Database.open(Paths.get(System.getenv("ML_INTEROP_PATH")))) {
            for (String line : Files.readAllLines(Paths.get(System.getenv("ML_FIXTURE")))) {
                if (line.isEmpty() || line.startsWith("#")) continue;
                String[] fields = line.split("\t");
                if (!Arrays.equals(db.get(unhex(fields[0])), unhex(fields[1])))
                    throw new AssertionError("JVM fixture mismatch");
            }
        }
    }
}
