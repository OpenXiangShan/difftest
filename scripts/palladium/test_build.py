#!/usr/bin/env python3
"""License-free PLDM regressions: python3 scripts/palladium/test_build.py."""

import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tarfile
import tempfile
import time
import unittest


ROOT = Path(__file__).resolve().parents[2]


class PalladiumBuildTest(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        self.env = os.environ.copy()
        self.env.pop("MAKEFLAGS", None)
        self.env.pop("MFLAGS", None)
        for directory in ("bin", "rtl", "generated", "csrc", "src/test/vsrc/vcs", "src/test/vsrc/common"):
            (self.root / directory).mkdir(parents=True)
        self.write("bin/cds_root", '#!/bin/sh\nprintf "%s\\n" "$PWD"\n', executable=True)
        for tool in ("vlan", "ixcom", "ixclkgen", "xeDebug"):
            self.write(f"bin/{tool}", '#!/bin/sh\necho "Unexpected hardware command: $0" >&2\nexit 99\n', executable=True)
        self.env["PATH"] = str(self.root / "bin") + os.pathsep + self.env["PATH"]
        self.write("Makefile", f"""BUILD_DIR := $(CURDIR)/build
RTL_DIR := $(CURDIR)/rtl
GEN_VSRC_DIR := $(CURDIR)/generated
SIM_TOP := SimTop
SIM_CXXFILES := $(CURDIR)/a/same.cpp $(CURDIR)/b/same.cpp
SIM_LDFLAGS := -lz -Wl,--no-undefined
include {ROOT}/palladium.mk
""")
        self.write("a/nested.h", "#define OFFSET 3\n")
        self.write("a/value.h", '#include "nested.h"\n')
        self.write("a/same.cpp", '#include <zlib.h>\n#include "value.h"\nextern int helper();\n'
                   'extern "C" int answer() { return compressBound(23) + OFFSET + helper(); }\n')
        self.write("b/same.cpp", "int helper() { return 7; }\n")
        self.library = self.root / "build/pldm-compile/libdpi_emu.so"

    def write(self, name, text, executable=False):
        path = self.root / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(text)
        if executable:
            path.chmod(0o755)
        return path

    def run_command(self, command):
        result = subprocess.run(command, cwd=self.root, env=self.env, text=True,
                                stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        self.assertEqual(result.returncode, 0, f"{' '.join(map(str, command))}\n{result.stdout}")
        return result.stdout

    def make(self, target):
        return self.run_command(["make", "--no-print-directory", "-j2", target,
                                 f"PLDM_CSRC_DIR={self.root}/csrc"])

    def answer(self):
        # A fresh process avoids dlopen reusing an earlier version of the library.
        return int(self.run_command([sys.executable, "-c",
                                    "import ctypes, sys; print(ctypes.CDLL(sys.argv[1]).answer())",
                                    str(self.library)]))

    def test_software_only_link_and_incremental_rebuild(self):
        self.make("pldm-lib")
        original = self.answer()
        objects = list((self.root / "build/pldm-compile/cc_obj").rglob("*.o"))
        self.assertEqual(len(objects), 2, "Sources with equal basenames must have separate objects")
        timestamp = self.library.stat().st_mtime_ns
        self.make("pldm-lib")
        self.assertEqual(self.library.stat().st_mtime_ns, timestamp, "Unchanged sources must not relink")

        # Let source timestamps advance on filesystems with coarse timestamp resolution.
        time.sleep(1.1)
        self.write("b/same.cpp", "int helper() { return 9; }\n")
        self.make("pldm-lib")
        self.assertEqual(self.answer(), original + 2, "Changed C++ must reach the loaded SO")
        time.sleep(1.1)
        self.write("a/nested.h", "#define OFFSET 8\n")
        self.make("pldm-lib")
        self.assertEqual(self.answer(), original + 7, "Changed transitive headers must rebuild their source")

    def test_packaged_clock_gate_and_other_rtl_are_preserved(self):
        packaged = self.write("rtl/DifftestClockGate.v", "// packaged\n")
        common = self.write("src/test/vsrc/common/DifftestClockGate.v", "// common\n")
        unrelated = [self.write("rtl/Other.v", "// first\n"),
                     self.write("generated/Other.v", "// second\n")]
        vfiles = self.root / "build/pldm-compile/vfiles.f"
        self.make(str(vfiles))
        paths = vfiles.read_text().splitlines()
        self.assertEqual(paths.count(str(packaged)), 1)
        self.assertNotIn(str(common), paths)
        for path in unrelated:
            self.assertIn(str(path), paths, "Different paths with equal filenames must remain")

    def test_common_clock_gate_fallback(self):
        common = self.write("src/test/vsrc/common/DifftestClockGate.v", "// common\n")
        vfiles = self.root / "build/pldm-compile/vfiles.f"
        self.make(str(vfiles))
        self.assertEqual(vfiles.read_text().splitlines(), [str(common)])

    def test_clock_gate_removed_from_existing_filelist(self):
        packaged = self.write("rtl/DifftestClockGate.v", "// packaged\n")
        common = self.write("src/test/vsrc/common/DifftestClockGate.v", "// common\n")
        vfiles = self.root / "build/pldm-compile/vfiles.f"
        self.make(str(vfiles))
        self.make(str(vfiles))
        self.assertEqual(vfiles.read_text().splitlines(), [str(packaged)], "Regeneration must not append duplicates")
        packaged.unlink()
        self.make(str(vfiles))
        self.assertEqual(vfiles.read_text().splitlines(), [str(common)], "Removing packaged RTL must restore fallback")

    def test_release_archive_and_wrapper(self):
        cpu = self.root / "cpu"
        diff = cpu / "difftest"
        diff.mkdir(parents=True)
        for name in ("Makefile", "palladium.mk", "fpga.mk", "scripts/palladium/Makefile",
                     "scripts/fpga/release.sh", "src/test/vsrc/common/DifftestClockGate.v"):
            target = diff / name
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(ROOT / name, target)
        self.write("cpu/Makefile", "# CPU generator fixture\n")
        profile = {"cpu": "Fixture", "cmdConfigs": ["--config", "FixtureConfig", "--difftest-config", "ESBIDUN"]}
        self.write("cpu/build/generated-src/difftest_profile.json", json.dumps(profile, indent=2))
        self.write("cpu/build/rtl/SimTop.sv", "module SimTop; endmodule\n")
        self.write("cpu/difftest/worktrees/private.txt", "must not be packaged\n")
        for repository in (diff, cpu):
            self.run_command(["git", "-C", str(repository), "init", "-q"])
            self.run_command(["git", "-C", str(repository), "add", "Makefile"])
            self.run_command(["git", "-C", str(repository), "-c", "user.name=Test", "-c",
                              "user.email=test@example.invalid", "-c", "commit.gpgsign=false",
                              "commit", "-qm", "fixture"])
        release_dir = self.root / "release"
        release_dir.mkdir()
        self.run_command(["bash", str(ROOT / "scripts/fpga/release.sh"), str(cpu), str(release_dir), "test"])
        archives = list(release_dir.glob("*.tar.gz"))
        self.assertEqual(len(archives), 1)
        extracted = self.root / "extracted"
        with tarfile.open(archives[0]) as archive:
            names = archive.getnames()
            self.assertFalse(any(".git" in Path(name).parts or "worktrees" in Path(name).parts for name in names))
            archive.extractall(extracted, **({"filter": "data"} if hasattr(tarfile, "data_filter") else {}))
        packaged = next(extracted.iterdir())
        for name in ("Makefile", "Makefile.cpu", "difftest/Makefile", "difftest/palladium.mk",
                     "difftest/fpga.mk", "difftest/scripts/palladium/Makefile",
                     "build/generated-src/difftest_profile.json"):
            self.assertTrue((packaged / name).is_file(), name)
        self.assertEqual((packaged / "Makefile").read_bytes(), (ROOT / "scripts/palladium/Makefile").read_bytes())
        self.assertEqual((packaged / "Makefile.cpu").read_text(), "# CPU generator fixture\n")
        self.assertEqual(json.loads((packaged / "build/generated-src/difftest_profile.json").read_text()), profile)
        (packaged / "difftest/Makefile").write_text(""".PHONY: pldm-lib
pldm-lib:
	@printf '%s\\n' '$(NUM_CORES)' '$(NO_DIFF)' '$(SYNTHESIS)' '$(DIFFTEST_PERFCNT)' '$(PLDM_EXTRA_ARGS)' '$(NOOP_HOME)'
""")
        output = self.run_command(["make", "--no-print-directory", "-C", str(packaged), "pldm-lib",
                                   "NUM_CORES=2", "NO_DIFF=1", "SYNTHESIS=1", "DIFFTEST_PERFCNT=1",
                                   "PLDM_EXTRA_ARGS=+no-diff +max-cycles=42"])
        self.assertEqual(output.splitlines()[-6:], ["2", "1", "1", "1", "+no-diff +max-cycles=42", str(packaged)])

    def test_no_diff_deferred_result_compiles(self):
        self.write("config.h", "#define NUM_CORES 1\n#define CONFIG_NO_DIFFTEST\n"
                   "#define CONFIG_DIFFTEST_DEFERRED_RESULT\n")
        self.write("svdpi.h", 'typedef void *svScope;\nextern "C" svScope svGetScope();\n'
                   'extern "C" svScope svSetScope(svScope);\n')
        self.run_command(["g++", "-std=c++11", "-fsyntax-only", f"-I{self.root}",
                          f"-I{ROOT}/src/test/csrc/common", str(ROOT / "src/test/csrc/vcs/vcs_main.cpp")])

    @unittest.skipUnless(shutil.which("verilator"), "Verilator is required for clock-gate lint")
    def test_clock_gate_platform_branches(self):
        self.write("DifftestMacros.svh", "`define CONFIG_DIFFTEST_CLOCKGATE\n")
        clock_gate = ROOT / "src/test/vsrc/common/DifftestClockGate.v"
        command = ["verilator", "--lint-only", "--top-module", "DifftestClockGate", f"-I{self.root}"]
        self.run_command(command + ["-DPALLADIUM", "-DSYNTHESIS", str(clock_gate)])
        self.run_command(command + [str(clock_gate)])
        stub = self.write("BUFGCE.sv", "module BUFGCE(input I, input CE, output O); assign O = I & CE; endmodule\n")
        self.run_command(command + ["-DSYNTHESIS", str(clock_gate), str(stub)])


if __name__ == "__main__":
    unittest.main(verbosity=2)
