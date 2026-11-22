use multileveldb::*;
fn unhex(s: &str) -> Vec<u8> {
    if s == "-" {
        return vec![];
    }
    (0..s.len())
        .step_by(2)
        .map(|i| u8::from_str_radix(&s[i..i + 2], 16).unwrap())
        .collect()
}
#[test]
fn write_shared_fixture() -> Result<()> {
    let Some(path) = std::env::var_os("ML_INTEROP_PATH") else {
        return Ok(());
    };
    let mut options = OpenOptions::new()?;
    options
        .create_if_missing(true)?
        .compression(Compression::Snappy)?;
    let db = Database::open(path, Some(&options))?;
    let fixture = std::fs::read_to_string(
        std::env::var_os("ML_FIXTURE").expect("ML_FIXTURE is required for interop"),
    )
    .unwrap();
    for line in fixture
        .lines()
        .filter(|l| !l.starts_with('#') && !l.is_empty())
    {
        let (k, v) = line.split_once('\t').unwrap();
        db.put(&unhex(k), &unhex(v), None)?;
    }
    Ok(())
}
