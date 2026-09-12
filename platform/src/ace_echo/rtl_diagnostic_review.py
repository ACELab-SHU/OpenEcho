"""Explicit qualified diagnostic review; the native RTL adapter stays strict.

This entry emits a sidecar, never rewrites a native report, and never performs
or substitutes for full-output comparison. Omitting --enable returns DISABLED.
"""
from __future__ import annotations

import argparse
import json
from pathlib import Path
from . import venus1_mbist_classifier as classifier


def evaluate(*, enabled=False, request_path=None, request_sha256=None,
             qualification_path=None, qualification_sha256=None):
    if not enabled:
        return classifier.classify_native_run(enabled=False)
    path = classifier.checked(qualification_path,qualification_sha256)
    qualification = json.loads(path.read_text())
    classifier.require(qualification['schema']=='ace-echo-infrastructure-qualification/v1'
        and qualification['status']=='PASS','infrastructure qualification missing')
    classifier.require(qualification['classifier']['sha256']==classifier.sha256(Path(classifier.__file__)),
        'qualified classifier differs')
    classifier.require(qualification['original_native_reports_unchanged']
        and qualification['default_rtl_gate_unchanged'] and not qualification['rtl_modified'],
        'unqualified native gate changes')
    classifier.require(len(qualification['cases'])==2
        and {c['masked'] for c in qualification['cases']}=={False,True},'paired qualification missing')
    for case in qualification['cases']:
        payload=case['complete_tx_payload']
        classifier.require(case['native_status']=='FAIL'
            and case['diagnostic_classification']=='PASS_DIAGNOSTIC_CLASSIFICATION_ONLY'
            and payload['status']=='PASS_EXACT_DIAGNOSTIC'
            and payload['all_accepted_write_bytes']==8200 and payload['valid_iq_samples']==2048
            and not payload['output_omitted_or_reordered'] and case['preceding_dag_exact_bytes']==5184,
            'qualification lacks complete independent byte evidence')
        deliveries=case['cpu_tx_deliveries']
        classifier.require(len(deliveries)==(0 if case['masked'] else 2)
            and all(d['masked']==0 and d['state_before']==1 for d in deliveries),
            'qualification lacks paired CPU delivery evidence')
    classifier.require(bool(qualification['tests']) and all(t['returncode']==0 for t in qualification['tests']),
        'qualification tests did not pass')
    for test in qualification['tests']:
        classifier.checked(test['log'],test['sha256'])
    classifier.require(bool(qualification['evidence']),'qualification has no evidence')
    for p,digest in qualification['evidence'].items():
        classifier.checked(p,digest)
    request = json.loads(classifier.checked(request_path,request_sha256).read_text())
    classifier.require(request['inputs']['contract']['sha256']==qualification['contract']['sha256'],
        'request uses an unqualified diagnostic contract')
    result = classifier.classify_native_run(enabled=True,request_path=request_path,request_sha256=request_sha256)
    classifier.checked(path,qualification_sha256)
    result['infrastructure_qualification'] = dict(path=str(path.resolve()),sha256=qualification_sha256)
    result['native_gate_policy'] = 'UNCHANGED; sidecar classification is not application acceptance'
    return result


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--enable',action='store_true')
    p.add_argument('--request',type=Path)
    p.add_argument('--request-sha256')
    p.add_argument('--qualification',type=Path)
    p.add_argument('--qualification-sha256')
    p.add_argument('--output',type=Path,required=True)
    a=p.parse_args()
    if a.output.exists():
        p.error('refusing to overwrite an existing result')
    if a.enable and any(x is None for x in (a.request,a.request_sha256,a.qualification,a.qualification_sha256)):
        p.error('explicit enable requires hash-bound request and qualification')
    try:
        result=evaluate(enabled=a.enable,request_path=a.request,request_sha256=a.request_sha256,
            qualification_path=a.qualification,qualification_sha256=a.qualification_sha256)
    except Exception as exc:
        result=dict(status='FAIL_CLASSIFICATION',error=str(exc),native_result_unchanged=True,application_complete=False)
    a.output.parent.mkdir(parents=True,exist_ok=True)
    with a.output.open('x') as stream:
        stream.write(json.dumps(result,indent=2)+'\n')
    print(json.dumps(dict(path=str(a.output.resolve()),status=result['status'],error=result.get('error'))))
    return 0 if result['status'] in ('DISABLED','PASS_DIAGNOSTIC_CLASSIFICATION_ONLY') else 1


if __name__=='__main__':
    raise SystemExit(main())
