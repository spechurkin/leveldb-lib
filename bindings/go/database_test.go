package multileveldb

import (
	"bytes"
	"encoding/hex"
	"errors"
	"os"
	"path/filepath"
	"strings"
	"sync"
	"testing"
)

func must(t *testing.T, err error) {
	t.Helper()
	if err != nil {
		t.Fatal(err)
	}
}

func code(t *testing.T, err error, expected Code) {
	t.Helper()
	var native *Error
	if !errors.As(err, &native) || native.Code != expected {
		t.Fatalf("expected %v, got %v", expected, err)
	}
}

func equal(t *testing.T, db *Database, key, expected []byte, opts *ReadOptions, snapshot *Snapshot) {
	t.Helper()
	actual, found, err := db.Get(key, opts, snapshot)
	must(t, err)
	if !found || !bytes.Equal(actual, expected) {
		t.Fatal("binary value mismatch")
	}
}

func TestDatabase(t *testing.T) {
	path := filepath.Join(t.TempDir(), "данные-数据库-😀")
	zero, compression, fill := uint64(0), Snappy, false
	opts := &OpenOptions{ParanoidChecks: true, WriteBufferSize: 65536, MaxOpenFiles: 64,
		BlockSize: 1024, BlockRestartInterval: 8, Compression: &compression, CacheSize: &zero}
	db, err := Create(path, opts)
	must(t, err)
	defer db.Close()
	if opts.CreateIfMissing || opts.ErrorIfExists {
		t.Fatal("Create mutated options")
	}
	missing, found, err := db.Get([]byte("missing"), nil, nil)
	must(t, err)
	if missing != nil || found {
		t.Fatal("missing key")
	}
	must(t, db.Put(nil, nil, nil))
	empty, found, err := db.Get(nil, nil, nil)
	must(t, err)
	if empty == nil || len(empty) != 0 || !found {
		t.Fatal("empty value distinction")
	}
	key, value := []byte{0, 255, 128}, bytes.Repeat([]byte{0, 128, 255}, 350000)
	must(t, db.Put(key, value, &WriteOptions{Sync: true}))
	equal(t, db, key, value, &ReadOptions{VerifyChecksums: true, FillCache: &fill}, nil)
	snapshot, err := db.CreateSnapshot()
	must(t, err)
	defer snapshot.Close()
	batch, err := NewBatch()
	must(t, err)
	defer batch.Close()
	must(t, batch.Put(key, []byte("new")))
	must(t, batch.Put([]byte("z"), []byte("last")))
	must(t, batch.Delete(nil))
	must(t, db.Write(batch, &WriteOptions{Sync: true}))
	must(t, batch.Clear())
	must(t, db.Write(batch, nil))
	equal(t, db, key, []byte("new"), nil, nil)
	equal(t, db, key, value, nil, snapshot)
	must(t, batch.Close())
	code(t, db.Write(batch, nil), DatabaseClosed)
	cursor, err := db.Iterator(nil, snapshot)
	must(t, err)
	defer cursor.Close()
	valid, err := cursor.IsValid()
	must(t, err)
	if valid {
		t.Fatal("initial position")
	}
	_, err = cursor.Key()
	code(t, err, NotFound)
	must(t, snapshot.Close())
	must(t, cursor.SeekToFirst())
	k, err := cursor.Key()
	must(t, err)
	if len(k) != 0 {
		t.Fatal("empty key order")
	}
	must(t, cursor.Next())
	k, err = cursor.Key()
	must(t, err)
	if !bytes.Equal(k, key) {
		t.Fatal("binary order")
	}
	copied, err := cursor.Value()
	must(t, err)
	must(t, cursor.SeekToLast())
	must(t, cursor.Previous())
	must(t, cursor.Seek(key))
	v, err := cursor.Value()
	must(t, err)
	if !bytes.Equal(v, value) {
		t.Fatal("snapshot cursor")
	}
	must(t, cursor.Next())
	valid, err = cursor.IsValid()
	must(t, err)
	if valid {
		t.Fatal("end position")
	}
	must(t, cursor.Status())
	if !bytes.Equal(copied, value) {
		t.Fatal("copied buffer")
	}
	other, err := Create(filepath.Join(t.TempDir(), "other"), nil)
	must(t, err)
	defer other.Close()
	foreign, err := other.CreateSnapshot()
	must(t, err)
	defer foreign.Close()
	_, _, err = db.Get(key, nil, foreign)
	code(t, err, InvalidArgument)
	var workers sync.WaitGroup
	for i := 0; i < 4; i++ {
		workers.Add(1)
		go func(i int) {
			defer workers.Done()
			for n := 0; n < 50; n++ {
				if err := db.Put([]byte{100, byte(i), byte(n)}, key, nil); err != nil {
					t.Error(err)
					return
				}
			}
		}(i)
	}
	workers.Wait()
	must(t, db.Close())
	must(t, db.Close())
	_, _, err = db.Get(key, nil, nil)
	code(t, err, DatabaseClosed)
	must(t, cursor.SeekToFirst())
	locked, err := Open(path, nil)
	if err == nil {
		locked.Close()
		t.Fatal("cursor must retain lock")
	}
	must(t, cursor.Close())
	reopened, err := Open(path, nil)
	must(t, err)
	equal(t, reopened, key, []byte("new"), nil, nil)
	must(t, reopened.Delete(key, nil))
	must(t, reopened.Delete(key, nil))
	must(t, reopened.Close())
	created, err := Create(path, nil)
	if err == nil {
		created.Close()
		t.Fatal("Create must reject existing DB")
	}
	code(t, err, InvalidArgument)
	_, err = Open(filepath.Join(t.TempDir(), "absent"), opts)
	code(t, err, InvalidArgument)
}

func TestInvalidAndZeroHandles(t *testing.T) {
	for _, path := range []string{"", "bad\x00path", string([]byte{255})} {
		_, err := Create(path, nil)
		code(t, err, InvalidArgument)
	}
	var db Database
	code(t, db.Put(nil, nil, nil), DatabaseClosed)
	must(t, db.Close())
	var batch Batch
	code(t, batch.Clear(), DatabaseClosed)
	must(t, batch.Close())
	var cursor Cursor
	_, err := cursor.Key()
	code(t, err, DatabaseClosed)
	must(t, cursor.Close())
	var snapshot Snapshot
	must(t, snapshot.Close())
	_, err = Create(filepath.Join(t.TempDir(), "invalid"), &OpenOptions{MaxOpenFiles: 1 << 63})
	code(t, err, InvalidArgument)
}

func TestInterop(t *testing.T) {
	path := os.Getenv("ML_INTEROP_PATH")
	if path == "" {
		t.Skip("sequential integration driver only")
	}
	db, err := Open(path, nil)
	must(t, err)
	defer db.Close()
	fixture, err := os.ReadFile(os.Getenv("ML_FIXTURE"))
	must(t, err)
	decode := func(field string) []byte {
		if field == "-" {
			return []byte{}
		}
		out, err := hex.DecodeString(field)
		must(t, err)
		return out
	}
	for _, line := range strings.Split(string(fixture), "\n") {
		line = strings.TrimSpace(line)
		if line == "" || strings.HasPrefix(line, "#") {
			continue
		}
		fields := strings.Split(line, "\t")
		equal(t, db, decode(fields[0]), append(decode(fields[1]), 0, 254), nil, nil)
	}
	equal(t, db, []byte("fsharp\x00\xff"), []byte{0, 128, 255}, nil, nil)
	must(t, db.Put([]byte("go\x00\xff"), []byte{0, 128, 255}, nil))
}
