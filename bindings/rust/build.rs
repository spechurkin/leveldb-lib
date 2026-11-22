fn main() {
    println!("cargo:rerun-if-env-changed=ML_NATIVE_DIR");
    if let Some(dir) = std::env::var_os("ML_NATIVE_DIR") {
        println!(
            "cargo:rustc-link-search=native={}",
            std::path::Path::new(&dir).display()
        );
    }
    println!("cargo:rustc-link-lib=dylib=multileveldb");
}
