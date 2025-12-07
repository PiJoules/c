import unittest
import os
import subprocess
from pathlib import Path

BUILD_DIR = Path("build")
STAGE1_BIN = BUILD_DIR / "c"
STAGE2_BIN = BUILD_DIR / "c.stage2"
STAGE3_BIN = BUILD_DIR / "c.stage3"


class TestCompiler:
    def invoke(self, filename):
        obj = str(BUILD_DIR / Path(f"{filename}.o").name)
        res = subprocess.run([str(self.bin), filename, "-o", obj], capture_output=True)
        self.assertEqual(res.returncode, 0, res.args)

        res = subprocess.run(
            [os.environ.get("CC", "clang"), obj, "-o", str(BUILD_DIR / "a.out")],
            capture_output=True,
        )
        self.assertEqual(res.returncode, 0)

        res = subprocess.run([str(BUILD_DIR / "a.out")], capture_output=True)
        self.assertEqual(res.returncode, 0)

        return res.stdout.decode("utf-8")

    def test_hello_world(self):
        self.assertEqual(self.invoke("tests/hello_world.c"), "hello world\n")


class TestStage1Compiler(unittest.TestCase, TestCompiler):
    def setUp(self):
        self.bin = STAGE1_BIN


class TestStage2Compiler(unittest.TestCase, TestCompiler):
    def setUp(self):
        self.bin = STAGE2_BIN


class TestStage3Compiler(unittest.TestCase, TestCompiler):
    def setUp(self):
        self.bin = STAGE3_BIN


if __name__ == "__main__":
    unittest.main()
