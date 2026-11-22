"""Run each adapter in a separate process, closing the database between phases."""
import os
import shlex
import subprocess
import sys
import tempfile
from pathlib import Path

root = Path(__file__).resolve().parents[2]
build = Path(os.environ.get("ML_BUILD_DIR", root / "build")).resolve()
environment = os.environ.copy()
environment["ML_FIXTURE"] = str(root / "tests/fixtures/binary.tsv")
environment["PERL5LIB"] = str(root / "bindings/perl/lib")
environment["PYTHONPATH"] = os.pathsep.join(
    filter(None, [str(root / "bindings/python"), environment.get("PYTHONPATH")]))


def run(command):
    print("Running:", " ".join(map(str, command)), flush=True)
    subprocess.run(command, cwd=root, env=environment, check=True)


with tempfile.TemporaryDirectory(prefix="interop-", dir=build) as directory:
    environment["ML_INTEROP_PATH"] = str(Path(directory) / "db")
    cargo = shlex.split(os.environ.get("ML_CARGO", "cargo"))
    run(cargo + ["test", "-p", "multileveldb", "--test", "interop"])
    if os.environ.get("ML_JAVA_CLASSPATH"):
        run([os.environ.get("ML_JAVA", "java"), "-ea", "-cp", os.environ["ML_JAVA_CLASSPATH"],
             "io.multileveldb.InteropTest"])
    else:
        run([os.environ.get("ML_GRADLE", "gradle"), "-p", "bindings/jvm", "interopTest"])
    run(["node", "tests/integration/node.cjs"])
    run(["perl", "tests/integration/perl.pl"])
    run([sys.executable, "tests/integration/python.py"])
    run([os.environ.get("ML_RUBY", "ruby"), "-I", "bindings/ruby/lib", "tests/integration/ruby.rb"])
    dotnet = os.environ.get("ML_DOTNET", "dotnet")
    run([dotnet, "run", "--project", "tests/dotnet", "-c", "Release", "--", "--interop"])
    run([dotnet, "run", "--project", "tests/fsharp", "-c", "Release", "--", "--interop"])
    if os.environ.get("ML_SKIP_GO") != "1":
        run([os.environ.get("ML_GO", "go"), "-C", "bindings/go", "test", "-run", "^TestInterop$", "-count=1", "-v",
             "."])
    executable = build / ("interop_c.exe" if os.name == "nt" else "interop_c")
    if not executable.exists():
        executable = build / "Release" / executable.name
    run([str(executable)])
print("Rust -> JVM -> Node.js -> Perl -> Python -> Ruby -> C# -> F# -> "
      + ("" if os.environ.get("ML_SKIP_GO") == "1" else "Go -> ") + "C: passed")
