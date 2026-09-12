from pathlib import Path
import unittest
from unittest.mock import patch

from ace_echo.cli import parser as build_parser
from ace_echo.config import load_config
from ace_echo.backend import resolve_backend_profile
from ace_echo.doctor import Check, run_doctor
from ace_echo.forge import ForgeContractError, software_validation_mode, validate_backend_manifest

ROOT = Path(__file__).resolve().parents[1]


class ValidationPolicyTests(unittest.TestCase):
    def test_shipped_backends_use_fast_without_relaxing_rtl(self):
        for path in (ROOT / 'configs/backends').glob('*.json'):
            with self.subTest(backend=path.name):
                manifest = validate_backend_manifest(path)
                self.assertEqual(software_validation_mode(manifest), 'fast')
                policy = manifest['execution_policy']
                self.assertTrue(policy['rtl_only_after_full_software_pass'])
                self.assertTrue(policy['rtl_only_for_integrated_winner'])
                self.assertEqual(policy['diagnostic_engine'], 'gem5.verification')
                self.assertFalse(manifest['performance']['verification_profile_required_for_winner'])
                self.assertTrue(manifest['engines']['rtl']['source_immutable'])
                self.assertTrue(manifest['engines']['rtl']['fresh_build_required'])

    def test_legacy_policy_is_not_silently_weakened(self):
        self.assertEqual(software_validation_mode({}), 'verification')
        self.assertEqual(software_validation_mode({'execution_policy': {
            'full_software_engine': 'gem5.verification'}}), 'verification')

    def test_invalid_engine_rejected(self):
        for engine in ('rtl', 'gem5.opt', '', None, False):
            with self.subTest(engine=engine), self.assertRaises(ForgeContractError):
                software_validation_mode({'execution_policy': {'full_software_engine': engine}})

    def test_doctor_debug_is_optional_only_on_normal_fast_route(self):
        config = load_config(ROOT / 'configs/local.toml')
        backend = resolve_backend_profile(config, ROOT / 'configs/backends/venus2p0-16x128.json')
        def exists(name, path, kind='file'):
            return Check(name, 'FAIL' if name == 'Gem5 verification binary' else 'PASS', str(path), kind)
        with patch('ace_echo.doctor._exists', side_effect=exists), patch('ace_echo.doctor.shutil.which', return_value='/tools/tool'):
            for scope in ('fast', 'software', 'full'):
                self.assertEqual(run_doctor(config, backend, scope)['status'], 'PASS')
            self.assertEqual(run_doctor(config, backend, 'diagnostic')['status'], 'FAIL')
            backend.manifest['execution_policy']['full_software_engine'] = 'gem5.verification'
            for scope in ('software', 'full', 'diagnostic'):
                self.assertEqual(run_doctor(config, backend, scope)['status'], 'FAIL')
            self.assertEqual(run_doctor(config, backend, 'fast')['status'], 'PASS')

    def test_run_defaults_fast_and_explicit_debug_stays_debug(self):
        parser = build_parser()
        base = ['run', 'dag', '--dag-json', 'dag.json', '--combined-bin', 'dag.bin', '--case-dir', 'case']
        self.assertEqual(parser.parse_args(base).mode, 'fast')
        self.assertEqual(parser.parse_args(base + ['--mode', 'verification']).mode, 'verification')


if __name__ == '__main__':
    unittest.main()
