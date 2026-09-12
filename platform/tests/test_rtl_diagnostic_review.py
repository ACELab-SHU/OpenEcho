import copy
import importlib.util
import json
from pathlib import Path
import sys
import tempfile
import types
import unittest
from unittest.mock import patch
from ace_echo import venus1_mbist_classifier as classifier

from ace_echo import rtl_diagnostic_review as review


class ReviewEntryTests(unittest.TestCase):
    def fixture(self,folder):
        root=Path(folder)
        evidence=root/'evidence.txt'
        evidence.write_text('unit fixture; not real hardware evidence\n')
        contract_digest='a'*64
        req=root/'request.json'
        req.write_text(json.dumps(dict(inputs=dict(contract=dict(sha256=contract_digest)))))
        payload=dict(status='PASS_EXACT_DIAGNOSTIC',all_accepted_write_bytes=8200,
                     valid_iq_samples=2048,output_omitted_or_reordered=False)
        q=dict(schema='ace-echo-infrastructure-qualification/v1',status='PASS',
            classifier=dict(sha256=classifier.sha256(Path(classifier.__file__))),
            original_native_reports_unchanged=True,default_rtl_gate_unchanged=True,rtl_modified=False,
            cases=[dict(masked=bool(n),native_status='FAIL',diagnostic_classification='PASS_DIAGNOSTIC_CLASSIFICATION_ONLY',
                complete_tx_payload=copy.deepcopy(payload),preceding_dag_exact_bytes=5184,
                cpu_tx_deliveries=[] if n else [dict(masked=0,state_before=1),dict(masked=0,state_before=1)]) for n in (0,1)],
            tests=[dict(returncode=0,log=str(evidence),sha256=classifier.sha256(evidence))],
            evidence={str(evidence):classifier.sha256(evidence)},contract=dict(sha256=contract_digest))
        return root,req,q

    def attempt(self,edit=None):
        with tempfile.TemporaryDirectory(prefix='mbist-review-entry-') as folder:
            root,req,q=self.fixture(folder)
            if edit:
                edit(q)
            certificate=root/'qualification.json'
            certificate.write_text(json.dumps(q))
            with patch.object(classifier,'classify_native_run',return_value=dict(status='PASS_DIAGNOSTIC_CLASSIFICATION_ONLY')) as call:
                if edit:
                    with self.assertRaises((classifier.Rejected,KeyError)):
                        review.evaluate(enabled=True,request_path=req,request_sha256=classifier.sha256(req),
                            qualification_path=certificate,qualification_sha256=classifier.sha256(certificate))
                    call.assert_not_called()
                else:
                    r=review.evaluate(enabled=True,request_path=req,request_sha256=classifier.sha256(req),
                        qualification_path=certificate,qualification_sha256=classifier.sha256(certificate))
                    self.assertIn('UNCHANGED',r['native_gate_policy'])
                    call.assert_called_once()

    def test_qualified_explicit_entry(self):
        self.attempt()

    def test_default_disabled_without_reading(self):
        with patch.object(classifier,'checked',side_effect=AssertionError('unexpected file read')):
            self.assertEqual(review.evaluate()['status'],'DISABLED')

    def test_unqualified_status(self):
        self.attempt(lambda q:q.update(status='FAIL'))

    def test_different_classifier(self):
        self.attempt(lambda q:q['classifier'].update(sha256='0'*64))

    def test_changed_rtl(self):
        self.attempt(lambda q:q.update(rtl_modified=True))

    def test_native_rewrite(self):
        self.attempt(lambda q:q.update(original_native_reports_unchanged=False))

    def test_default_gate_changed(self):
        self.attempt(lambda q:q.update(default_rtl_gate_unchanged=False))

    def test_missing_masked_pair(self):
        self.attempt(lambda q:q['cases'].pop())

    def test_missing_bytes(self):
        self.attempt(lambda q:q['cases'][0]['complete_tx_payload'].update(all_accepted_write_bytes=8192))

    def test_missing_sample(self):
        self.attempt(lambda q:q['cases'][0]['complete_tx_payload'].update(valid_iq_samples=2047))

    def test_missing_cpu_irq(self):
        self.attempt(lambda q:q['cases'][0]['cpu_tx_deliveries'].pop())

    def test_extra_masked_irq(self):
        self.attempt(lambda q:q['cases'][1]['cpu_tx_deliveries'].append(dict(masked=1,state_before=1)))

    def test_wrong_contract(self):
        self.attempt(lambda q:q['contract'].update(sha256='b'*64))

    def test_tests_failed(self):
        self.attempt(lambda q:q['tests'][0].update(returncode=1))

    def test_no_evidence(self):
        self.attempt(lambda q:q.update(evidence={}))


if __name__=='__main__':
    unittest.main()
