use multileveldb::{Database, OpenOptions, Result};
fn main() -> Result<()> {
    let mut options=OpenOptions::new()?;
    options.create_if_missing(true)?;
    let db=Database::open("example-db",Some(&options))?;
    db.put(&[0,255],&[0,128,255],None)?;
    assert_eq!(db.get(&[0,255],None,None)?,Some(vec![0,128,255]));
    let snapshot=db.snapshot()?;
    let mut batch=multileveldb::Batch::new()?;
    batch.put(&[0,255],&[])?.put(b"other",b"value")?;
    db.write(&batch,None)?;
    assert_eq!(db.get(&[0,255],None,Some(&snapshot))?,Some(vec![0,128,255]));
    Ok(())
}
