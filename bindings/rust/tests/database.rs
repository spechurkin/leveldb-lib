use multileveldb::*;
#[test]
fn create_new_preserves_options_and_data() -> Result<()> {
    let root = std::env::var_os("ML_TEST_ROOT")
        .map(std::path::PathBuf::from)
        .unwrap_or_else(std::env::temp_dir)
        .join(format!("multileveldb-create-{}", std::process::id()));
    std::fs::create_dir(&root).unwrap();
    let path = root.join("новая");
    let mut options = OpenOptions::new()?;
    options
        .create_if_missing(false)?
        .error_if_exists(false)?
        .cache_size(0)?;
    {
        let db = Database::create(&path, Some(&options))?;
        let mut cursor = db.iterator(None, None)?;
        cursor.seek_to_first()?;
        assert!(!cursor.is_valid());
        db.put(b"\0\xff", b"\x80\0", None)?;
        assert!(
            matches!(Database::create(&path, None), Err(e) if e.code == ErrorCode::DatabaseLocked)
        );
    }
    assert!(
        matches!(Database::create(&path, Some(&options)), Err(e) if e.code == ErrorCode::InvalidArgument)
    );
    {
        let db = Database::open(&path, Some(&options))?;
        assert_eq!(db.get(b"\0\xff", None, None)?, Some(b"\x80\0".to_vec()));
    }
    assert!(
        matches!(Database::open(root.join("absent"), Some(&options)), Err(e) if e.code == ErrorCode::InvalidArgument)
    );
    drop(Database::create(root.join("defaults"), None)?);
    assert!(matches!(Database::create("", None), Err(e) if e.code == ErrorCode::InvalidArgument));
    std::fs::remove_dir_all(root).unwrap();
    Ok(())
}

#[test]
fn binary_lifetimes_and_reopen() -> Result<()> {
    let root = std::env::var_os("ML_TEST_ROOT")
        .map(std::path::PathBuf::from)
        .unwrap_or_else(std::env::temp_dir);
    let path = root.join(format!("multileveldb-rust-данные-{}", std::process::id()));
    let mut options = OpenOptions::new()?;
    options
        .error_if_exists(false)?
        .paranoid_checks(true)?
        .write_buffer_size(65536)?
        .max_open_files(40)?
        .block_size(4096)?
        .block_restart_interval(16)?
        .cache_size(1048576)?;
    let mut read = ReadOptions::new()?;
    read.verify_checksums(true)?.fill_cache(false)?;
    let mut write = WriteOptions::new()?;
    write.sync(true)?;
    options
        .create_if_missing(true)?
        .compression(Compression::None)?;
    {
        let db = Database::open(&path, Some(&options))?;
        db.put(b"\0\xff", b"\0\x80\xff", Some(&write))?;
        assert_eq!(
            db.get(b"\0\xff", Some(&read), None)?,
            Some(b"\0\x80\xff".to_vec())
        );
        assert!(matches!(Database::open(&path,None),Err(e) if e.code==ErrorCode::DatabaseLocked));
        let s = db.snapshot()?;
        let mut b = Batch::new()?;
        b.put(b"\0\xff", b"")?.put(b"z", b"v")?.delete(b"z")?;
        db.write(&b, None)?;
        assert_eq!(
            db.get(b"\0\xff", None, Some(&s))?,
            Some(b"\0\x80\xff".to_vec())
        );
        assert_eq!(db.get(b"\0\xff", None, None)?, Some(vec![]));
        let mut i = db.iterator(None, Some(&s))?;
        drop(s);
        i.seek_to_first()?;
        assert!(i.is_valid());
        assert_eq!(i.value()?, b"\0\x80\xff");
        i.seek_to_last()?;
        i.previous()?;
        assert!(!i.is_valid());
        i.seek(b"\0\xff")?;
        i.next()?;
        assert!(!i.is_valid());
        i.status()?;
        db.delete(b"\0\xff", None)?;
        assert_eq!(db.get(b"\0\xff", None, None)?, None);
        let large = vec![0xff; 4 * 1024 * 1024];
        db.put(b"large", &large, None)?;
        assert_eq!(db.get(b"large", None, None)?, Some(large));
        std::thread::scope(|scope| {
            for _ in 0..4 {
                scope.spawn(|| {
                    db.put(b"thread", b"yes", None).unwrap();
                });
            }
        });
    }
    let mut db = Database::open(&path, None)?;
    assert_eq!(db.get(b"thread", None, None)?, Some(b"yes".to_vec()));
    db.close()?;
    db.close()?;
    assert!(matches!(db.put(b"x",b"y",None),Err(e) if e.code==ErrorCode::DatabaseClosed));
    drop(db);
    std::fs::remove_dir_all(path).unwrap();
    Ok(())
}
