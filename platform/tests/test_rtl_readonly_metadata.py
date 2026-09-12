from pathlib import Path
import os
import shutil
import stat
import tempfile
import unittest
from unittest.mock import patch
from ace_echo.adapters.rtl import RtlAdapter


class ReadonlyMetadataTests(unittest.TestCase):
    def test_readonly_copy_is_relocated_without_mutating_source_or_mode(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            source = root / 'original.lst'
            source.write_text('/old/core.v\n')
            source.chmod(0o444)
            snapshot = root / 'snapshot'
            snapshot.mkdir()
            copied = snapshot / 'core.lst'
            shutil.copy2(source, copied)
            RtlAdapter._adapt_snapshot_metadata(snapshot, {'generated_path_rewrites':[
                {'glob':'*.lst','old':'/old','new':'{snapshot}'}]})
            self.assertEqual(copied.read_text(), str(snapshot)+'/core.v\n')
            self.assertEqual(source.read_text(), '/old/core.v\n')
            self.assertEqual(stat.S_IMODE(copied.stat().st_mode), 0o444)

    def test_external_symlink_and_hdl_edits_are_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            source = root / 'original.lst'
            source.write_text('original')
            snapshot = root / 'snapshot'
            snapshot.mkdir()
            link = snapshot / 'linked.lst'
            link.symlink_to(source)
            with self.assertRaises(ValueError):
                RtlAdapter._write_snapshot_metadata(snapshot, link, 'changed')
            hdl = snapshot / 'core.sv'
            hdl.write_text('module core; endmodule')
            with self.assertRaises(ValueError):
                RtlAdapter._write_snapshot_metadata(snapshot, hdl, 'changed')
            self.assertEqual(source.read_text(), 'original')
            self.assertEqual(hdl.read_text(), 'module core; endmodule')

    def test_shared_hardlink_cannot_mutate_original(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            source = root / 'original.lst'; source.write_text('original')
            snapshot = root / 'snapshot'; snapshot.mkdir()
            copied = snapshot / 'copied.lst'; os.link(source, copied)
            with self.assertRaisesRegex(ValueError, 'shared-inode'):
                RtlAdapter._write_snapshot_metadata(snapshot, copied, 'changed')
            self.assertEqual(source.read_text(), 'original')

    def test_write_failure_restores_readonly_mode(self):
        with tempfile.TemporaryDirectory() as tmp:
            snapshot = Path(tmp)
            copied = snapshot / 'copied.lst'; copied.write_text('original'); copied.chmod(0o444)
            with patch.object(Path, 'write_text', side_effect=OSError('write failed')):
                with self.assertRaises(OSError):
                    RtlAdapter._write_snapshot_metadata(snapshot, copied, 'changed')
            self.assertEqual(stat.S_IMODE(copied.stat().st_mode), 0o444)
            self.assertEqual(copied.read_text(), 'original')
