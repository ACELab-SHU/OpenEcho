from pathlib import Path
import ast
import json
import re
import tempfile
import unittest

from ace_echo.adapters.gem5 import Gem5Adapter
from ace_echo.adapters.toolchain import ToolchainAdapter
from ace_echo.backend import resolve_backend_profile
from ace_echo.config import ConfigError, load_config
from ace_echo.process import Runner


class BackendProfileTests(unittest.TestCase):
    @property
    def root(self) -> Path:
        return Path(__file__).resolve().parents[1]

    def profile(self, name: str):
        config = load_config(self.root / "configs/local.toml")
        return config, resolve_backend_profile(
            config, self.root / "configs/backends" / name
        )

    def test_venus1_selects_all_architecture_sensitive_components(self):
        """The explicit legacy manifest remains the 150 MHz compatibility baseline."""
        _, profile = self.profile("venus1p0-64x512.json")
        self.assertEqual(profile.backend_id, "venus1p0-64x512")
        self.assertEqual(profile.gem5.venus_config, "venus1p0-64x512")
        self.assertEqual(profile.toolchain_make_variables["VENUSROW"], "512")
        self.assertEqual(profile.toolchain_make_variables["VENUSLANE"], "64")
        self.assertEqual(
            profile.venus_llvm_bin,
            Path("/home/xusiyi/new/llvm-project-cmake-build-debug/bin"),
        )
        self.assertEqual(
            profile.rtl_root,
            Path("/home/shenyihao/Project/Venus/venus_soc"),
        )
        self.assertEqual(
            profile.scheduler_linker_script,
            self.root / "configs/backends/venus1p0-l1.ld",
        )
        self.assertFalse(profile.scheduler_configure_dcache_end)
        self.assertTrue(profile.scheduler_enable_ctrl_iopads)
        self.assertFalse(
            profile.manifest["abi"]["task_container"]
            ["includes_spmd_fields"]
        )
        self.assertEqual(
            profile.scheduler_pll_helper_body,
            self.root / "configs/backends/venus1p0-pll-helper.inc",
        )
        self.assertEqual(
            profile.scheduler_devctrl_init_body,
            self.root / "configs/backends/venus1p0-devctrl-init.inc",
        )
        self.assertEqual(
            profile.manifest["engines"]["rtl"]["flow"]
            ["fresh_build_target"],
            "sim_compileall",
        )
        dependencies = (
            profile.manifest["engines"]["rtl"]["flow"]
            ["snapshot_dependencies"]
        )
        self.assertEqual(
            dependencies["copy_overlays"][0]["leaf_subdirectories"],
            ["src"],
        )
        self.assertEqual(
            [
                item["variables"]["CC_FABRIC"]
                for item in dependencies["pre_build_make"]
            ],
            [
                "venus_cluster_axi4",
                "venus_gc0802_axi",
                "venus_gc0802_apb",
                "venus_gc0802_flash_apb",
                "venus_cluster_axi4_to_apb",
            ],
        )

    def test_venus1_300mhz_profile_selects_bpll_firmware_and_gem5(self):
        _, profile = self.profile("venus1p0-64x512-300mhz.json")
        self.assertEqual(profile.backend_id, "venus1p0-64x512-300mhz")
        self.assertEqual(profile.gem5.venus_config,
                         "venus1p0-64x512-300mhz")
        self.assertEqual(
            profile.scheduler_devctrl_init_body,
            self.root / "configs/backends/venus1p0-devctrl-init-300mhz.inc",
        )
        clocks = profile.manifest["clock_contracts"]
        self.assertEqual(clocks["active"], "ace_echo_bpll_300mhz")
        self.assertEqual(clocks["ace_echo_bpll_300mhz"]["system_axi_hz"],
                         150_000_000)
        self.assertEqual(clocks["ace_echo_bpll_300mhz"]["tile_hz"],
                         300_000_000)
        self.assertEqual(profile.clock_contract_id,
                         "ace_echo_bpll_300mhz")
        self.assertEqual(profile.system_axi_hz, 150_000_000)
        self.assertEqual(profile.tile_hz, 300_000_000)
        body = profile.scheduler_devctrl_init_body.read_text(encoding="utf-8")
        debug_values = re.findall(
            r"GC0802_DEBUG_OFFSET,\s*(0x[0-9a-fA-F]+)", body
        )
        self.assertEqual(debug_values[-1], "0x00000100")

    def test_venus1_clock_profiles_only_diverge_in_declared_surfaces(self):
        compatibility = json.loads(
            (self.root / "configs/backends/venus1p0-64x512.json")
            .read_text(encoding="utf-8")
        )
        performance = json.loads(
            (self.root / "configs/backends/venus1p0-64x512-300mhz.json")
            .read_text(encoding="utf-8")
        )
        for document in (compatibility, performance):
            document.pop("backend_id")
            document.pop("display_name")
            document.pop("clock_contracts")
            document["engines"]["scheduler"].pop("devctrl_init_body")
            document["engines"]["gem5"]["fast"].pop("venus_config")
            document["engines"]["gem5"]["verification"].pop("venus_config")
            document["performance"].pop("rtl_timing_status")
            document["performance"].pop("clock_domain", None)
        self.assertEqual(performance, compatibility)

    def test_venus2_profile_remains_the_default_architecture(self):
        _, profile = self.profile("venus2p0-16x128.json")
        self.assertEqual(profile.gem5.venus_config, "venus-rtl-16x128")
        self.assertEqual(profile.toolchain_make_variables["VENUSROW"], "128")
        self.assertEqual(profile.toolchain_make_variables["VENUSLANE"], "16")
        self.assertTrue(profile.scheduler_configure_dcache_end)
        self.assertFalse(profile.scheduler_enable_ctrl_iopads)
        self.assertTrue(
            profile.manifest["abi"]["task_container"]
            ["includes_spmd_fields"]
        )
        self.assertIsNone(profile.scheduler_pll_helper_body)
        self.assertIsNone(profile.scheduler_devctrl_init_body)

    def test_short_generation_aliases_select_checked_in_profiles(self):
        config = load_config(self.root / "configs/local.toml")
        self.assertEqual(
            resolve_backend_profile(config, Path("V1")).backend_id,
            "venus1p0-64x512-300mhz",
        )
        self.assertEqual(
            resolve_backend_profile(config, Path("V1-150")).backend_id,
            "venus1p0-64x512",
        )
        self.assertEqual(
            resolve_backend_profile(config, Path("V2")).backend_id,
            "venus2p0-16x128",
        )

    def test_toolchain_dry_run_carries_venus1_rows_and_lanes(self):
        config, profile = self.profile("venus1p0-64x512.json")
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            runner = Runner(root / "commands", dry_run=True)
            ToolchainAdapter(config, runner, profile).compile_dag(
                target="nrPBCH", output=root / "artifacts", timeout=30
            )
            command_record = next(
                (root / "commands").glob("*-compile-dag.json")
            )
            command = json.loads(command_record.read_text())["argv"]
            self.assertIn("VENUSROW=512", command)
            self.assertIn("VENUSLANE=64", command)
            self.assertIn(
                "LLVM_PATH=/home/xusiyi/new/llvm-project-cmake-build-debug/bin",
                command,
            )

    def test_gem5_dry_run_carries_venus1_profile(self):
        config, profile = self.profile("venus1p0-64x512.json")
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            runner = Runner(root / "commands", dry_run=True)
            Gem5Adapter(config, runner, profile).run_dag(
                dag_json=root / "dag.json",
                combined_bin=root / "dag.bin",
                case_dir=root / "tasks",
                output=root / "gem5",
                mode="fast",
                timeout=30,
            )
            command = json.loads(
                (root / "commands/01-run-dag.json").read_text()
            )["argv"]
            index = command.index("--venus-config")
            self.assertEqual(command[index + 1], "venus1p0-64x512")

    def test_gem5_launchers_accept_venus1_300mhz_profile(self):
        for relative in (
            "components/gem5/configs/tutorial/part1/packet_gen.py",
            "components/gem5/tools/venus_dag.py",
        ):
            source = (self.root / relative).read_text(encoding="utf-8")
            self.assertIn("venus1p0-64x512-300mhz", source)

    def test_manifest_cannot_conflict_with_architecture_dimensions(self):
        config, _ = self.profile("venus1p0-64x512.json")
        original = json.loads(
            (self.root / "configs/backends/venus1p0-64x512.json").read_text()
        )
        original["engines"]["toolchain"]["make_variables"]["VENUSROW"] = 128
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "backend.json"
            path.write_text(json.dumps(original), encoding="utf-8")
            with self.assertRaisesRegex(ConfigError, "conflicts"):
                resolve_backend_profile(config, path)

    def test_venus1_issue_timing_is_backend_scoped(self):
        """Keep RTL-derived V1 timing boundaries out of shared defaults."""
        source = (
            self.root
            / "components/gem5/configs/tutorial/part1/packet_gen.py"
        ).read_text(encoding="utf-8")
        module = ast.parse(source)
        assignment = next(
            node for node in module.body
            if isinstance(node, ast.Assign)
            and any(
                isinstance(target, ast.Name)
                and target.id == "VENUS_CONFIGS"
                for target in node.targets
            )
        )
        profiles = ast.literal_eval(assignment.value)

        self.assertEqual(
            profiles["venus1p0-64x512"]
            ["rtl_pe_command_visibility_cycles"],
            3,
        )
        self.assertNotIn(
            "rtl_pe_command_visibility_cycles",
            profiles["venus-rtl-16x128"],
        )
        self.assertEqual(
            profiles["venus1p0-64x512"]
            ["task_done_visibility_cycles"],
            1,
        )
        self.assertTrue(
            profiles["venus1p0-64x512"]
            ["task_done_visibility_uses_tile_clock"],
        )
        self.assertTrue(
            profiles["venus1p0-64x512"]
            ["reset_task_pipeline_before_fire"],
        )
        self.assertEqual(
            profiles["venus1p0-64x512"]["task_reset_vector"],
            0,
        )
        self.assertNotIn(
            "task_done_visibility_cycles",
            profiles["venus-rtl-16x128"],
        )
        self.assertNotIn(
            "task_done_visibility_uses_tile_clock",
            profiles["venus-rtl-16x128"],
        )
        self.assertNotIn(
            "reset_task_pipeline_before_fire",
            profiles["venus-rtl-16x128"],
        )
        self.assertNotIn(
            "task_reset_vector",
            profiles["venus-rtl-16x128"],
        )

    def test_direct_return_still_observes_target_vfu_capacity(self):
        """The Venus1 FSM shortcut must not bypass the physical VFU Q."""
        source = (
            self.root / "components/gem5/src/venus/VenusSequencer.cc"
        ).read_text(encoding="utf-8")

        self.assertIn(
            "const bool returnReady = currentIssueVfuQueuesReady();",
            source,
        )
        self.assertNotIn(
            "rtlDirectDownstreamReturn\n"
            "                ? !rtlLaneDesyncStall()",
            source,
        )


if __name__ == "__main__":
    unittest.main()
