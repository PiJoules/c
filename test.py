import unittest
import os
import subprocess


STAGE1_BIN = "build/c"
STAGE2_BIN = "build/c.stage2"


class TestStringMethods(unittest.TestCase):

    def invoke(self, filename):
        res = subprocess.run([STAGE1_BIN, filename], capture_output=True)
        self.assertEqual(res.returncode, 0)

        res = subprocess.run(
            [os.environ.get("CC", "clang"), "out.obj", "-o", "a.out"],
            capture_output=True,
        )
        self.assertEqual(res.returncode, 0)

        res = subprocess.run(["./a.out"], capture_output=True)
        self.assertEqual(res.returncode, 0)

        return res.stdout.decode("utf-8")

    def test_hello_world(self):
        self.assertEqual(self.invoke("tests/hello_world.c"), "hello world\n")


if __name__ == "__main__":
    unittest.main()
