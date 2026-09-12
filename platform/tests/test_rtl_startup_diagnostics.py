from pathlib import Path
import hashlib
import tempfile
import unittest
from ace_echo.rtl_startup_diagnostics import classify_startup_errors


class StartupDiagnosticTests(unittest.TestCase):
    def setUp(self):
        self.tmp=tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root=Path(self.tmp.name)
        self.source=self.root/'devctrl.sv'
        self.source.write_text('known version')
        self.rule=dict(id='startup',source_file='devctrl.sv',source_sha256=hashlib.sha256(self.source.read_bytes()).hexdigest(),
                       instance='tb.dut',message='decode toggle',max_occurrences=1)

    def lines(self,time='0 fs'):
        return [f'Error: "{self.source}", 9: tb.dut: at time {time}','decode toggle']

    def classify(self,lines): return classify_startup_errors(lines,[self.rule],self.root)

    def test_exact_time_zero_units(self):
        for unit in ('fs','ps','ns','us','ms','s'):
            with self.subTest(unit=unit): self.assertEqual(len(self.classify(self.lines('0 '+unit))),1)

    def test_later_error_is_not_waived(self): self.assertEqual(self.classify(self.lines('0.001 fs')),[])
    def test_repeated_error_is_not_waived(self): self.assertEqual(self.classify(self.lines()*2),[])
    def test_wrong_source_digest_is_not_waived(self):
        self.source.write_text('changed version')
        self.assertEqual(self.classify(self.lines()),[])
    def test_other_message_is_not_waived(self): self.assertEqual(self.classify([self.lines()[0],'memory failure']),[])
    def test_other_instance_is_not_waived(self): self.assertEqual(self.classify([self.lines()[0].replace('tb.dut:','tb.other:'),'decode toggle']),[])
    def test_no_source_evidence_does_not_waive(self): self.assertEqual(classify_startup_errors(self.lines(),[self.rule],None),[])
    def test_path_escape_is_rejected(self):
        self.rule['source_file']='../devctrl.sv'
        with self.assertRaises(ValueError): self.classify(self.lines())
