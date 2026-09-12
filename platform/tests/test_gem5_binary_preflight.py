from pathlib import Path
from types import SimpleNamespace
import tempfile
import unittest
from unittest.mock import Mock

from ace_echo.adapters.gem5 import Gem5Adapter


class Gem5BinaryPreflightTests(unittest.TestCase):
    def adapter(self, root, dry=False):
        adapter=object.__new__(Gem5Adapter)
        adapter.root=root
        adapter.runner=SimpleNamespace(dry_run=dry,run=Mock())
        adapter.gem5=SimpleNamespace(binary=lambda mode: root/('gem5.debug' if mode=='verification' else 'gem5.opt'))
        return adapter

    def test_missing_verification_never_falls_back_to_fast(self):
        with tempfile.TemporaryDirectory() as tmp:
            root=Path(tmp); adapter=self.adapter(root)
            opt=root/'gem5.opt';opt.write_text('fixture');opt.chmod(0o755)
            with self.assertRaisesRegex(FileNotFoundError,'--mode debug --jobs 4'):
                adapter._require_binary('verification')
            self.assertEqual(adapter._require_binary('fast'),opt)
            adapter.runner.run.assert_not_called()

    def test_dag_fails_before_hydration_and_dry_run_is_allowed(self):
        with tempfile.TemporaryDirectory() as tmp:
            root=Path(tmp); adapter=self.adapter(root)
            with self.assertRaisesRegex(FileNotFoundError,'Gem5 fast'):
                adapter.run_dag(dag_json=root/'missing.json',combined_bin=root/'missing.bin',
                                case_dir=root,output=root/'out',mode='fast',timeout=1)
            self.assertFalse((root/'out').exists())
            adapter.runner.run.assert_not_called()
            self.assertEqual(self.adapter(root,True)._require_binary('verification'),root/'gem5.debug')

    def test_nonexecutable_is_not_a_valid_installation(self):
        with tempfile.TemporaryDirectory() as tmp:
            root=Path(tmp); binary=root/'gem5.debug';binary.write_text('fixture');binary.chmod(0o644)
            with self.assertRaisesRegex(FileNotFoundError,'not executable'):
                self.adapter(root)._require_binary('verification')


if __name__=='__main__':
    unittest.main()
