package main

import (
	"bytes"
	"fmt"
	"log"

	"github.com/spechurkin/leveldb-lib/bindings/go"
)

func example() error {
	db, err := multileveldb.Open("example-db", &multileveldb.OpenOptions{CreateIfMissing: true})
	if err != nil {
		return err
	}
	defer db.Close()
	key, value := []byte{0, 255}, []byte{0, 128, 255}
	if err := db.Put(key, value, nil); err != nil {
		return err
	}
	actual, found, err := db.Get(key, nil, nil)
	if err != nil {
		return err
	}
	if !found || !bytes.Equal(actual, value) {
		return fmt.Errorf("roundtrip failed")
	}
	cursor, err := db.Iterator(nil, nil)
	if err != nil {
		return err
	}
	defer cursor.Close()
	if err := cursor.SeekToFirst(); err != nil {
		return err
	}
	for {
		valid, err := cursor.IsValid()
		if err != nil {
			return err
		}
		if !valid {
			break
		}
		key, err := cursor.Key()
		if err != nil {
			return err
		}
		value, err := cursor.Value()
		if err != nil {
			return err
		}
		fmt.Printf("%x %x\n", key, value)
		if err := cursor.Next(); err != nil {
			return err
		}
	}
	return cursor.Status()
}

func main() {
	if err := example(); err != nil {
		log.Fatal(err)
	}
}
