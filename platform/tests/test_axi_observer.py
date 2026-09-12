from copy import deepcopy
import json
from pathlib import Path
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import patch

from ace_echo.axi_observer import AxiObserverAdapter, checked, sha256, validate_contract


BASE = json.loads((Path(__file__).parents[1] / "configs/observers/venus1p0-l1-axi-v1.json").read_text())


class ObserverIdentityTests(unittest.TestCase):
    def setUp(self):
        temp = tempfile.TemporaryDirectory()
        self.addCleanup(temp.cleanup)
        self.root = Path(temp.name)
        self.source = self.root / "backend.json"
        self.source.write_text("{}")
        self.rtl_source = self.root / "type-source-identity.txt"
        # This is a byte-identity fixture, not an HDL model or simulation input.
        self.rtl_source.write_text("read-only identity fixture")
        self.backend = SimpleNamespace(backend_id=BASE["backend_id"], source=self.source, rtl_root=self.root)
        self.contract = deepcopy(BASE)
        self.contract["backend_sha256"] = sha256(self.source)
        self.contract["rtl_sources"] = {self.rtl_source.name: sha256(self.rtl_source)}
        self.path = self.root / "contract.json"

    def validate(self):
        self.path.write_text(json.dumps(self.contract))
        with patch("ace_echo.axi_observer.git_identity", return_value={"head": BASE["rtl_commit"]}):
            return validate_contract(self.backend, self.path, sha256(self.path))

    def test_matching_identity_passes(self):
        self.assertEqual(self.validate(), self.contract)

    def test_firmware_and_evidence_digest_mismatch_fail(self):
        for label in ("firmware.bin", "l1-axi.vpd", "transactions.json"):
            path = self.root / label
            path.write_bytes(b"original")
            digest = sha256(path)
            self.assertEqual(checked(path, digest), path)
            path.write_bytes(b"changed")
            with self.assertRaisesRegex(ValueError, "identity mismatch"):
                checked(path, digest)

    def test_backend_rtl_source_and_commit_mismatch_fail(self):
        for key in ("backend_sha256", "rtl_commit"):
            old = self.contract[key]
            self.contract[key] = "0"*64
            with self.assertRaises(ValueError):
                self.validate()
            self.contract[key] = old
        self.rtl_source.write_text("changed")
        with self.assertRaisesRegex(ValueError, "identity mismatch"):
            self.validate()

    def test_unsafe_hierarchy_signal_and_source_paths_fail(self):
        for key, value in (("ucli_hierarchy", "tb.dma; force bad 1"),
                           ("signals", {"bad;run": 1}),
                           ("rtl_sources", {"../outside": "0"*64})):
            old = self.contract[key]
            self.contract[key] = value
            with self.assertRaises(ValueError):
                self.validate()
            self.contract[key] = old

    def test_out_of_bounds_field_fails(self):
        self.contract["fields"]["wvalid"] = ["axi_req_o", 712, 1]
        with self.assertRaisesRegex(ValueError, "out-of-range"):
            self.validate()


class ObserverLaunchTests(unittest.TestCase):
    def test_only_new_script_and_compile_override_change(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "sim").mkdir()
            makefile = root / "sim/Makefile"
            text = 'VCS_FLAGS_NODEBUG="-V -full64 -timescale=${VCS_TIMESCALE}"\n'
            makefile.write_text(text)
            flow = {"make_variables": {"SIM_TYPE": "simonly"}}
            backend = SimpleNamespace(rtl_root=root, manifest={"engines": {"rtl": {"flow": flow}}})
            adapter = AxiObserverAdapter(None, backend, BASE)
            original = "suppress_message WARNING\nrun 1ps\nunsuppress_message WARNING\nrun 1000000us\nexit\n"
            snapshot = root / "new-snapshot"
            (snapshot / "sim").mkdir(parents=True)
            script = snapshot / "sim/simv_ucli.tcl"
            script.write_text(original)
            adapter._set_sim_horizon(snapshot, 200000)
            captured = script.read_text()
            self.assertTrue(captured.endswith(original.replace("1000000us", "200000us")))
            self.assertIn("dump -deltaCycle on", captured)
            self.assertEqual(captured.count("tb.pad.dut.u_venus_L1_dmac.u_dma_func."), 8)
            self.assertNotIn("force", captured)
            self.assertNotIn("deposit", captured)
            self.assertEqual(makefile.read_text(), text)
            self.assertEqual(flow["make_variables"], {"SIM_TYPE": "simonly"})


if __name__ == "__main__":
    unittest.main()
