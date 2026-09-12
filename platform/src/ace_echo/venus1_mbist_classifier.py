"""Opt-in Venus1 diagnostic classification. Never changes native RTL results.

Full VCD transition lists are retained within each timestamp; guard checks do
not use just clock samples or the final value after a same-timestamp pulse.
The APB judge independently uses pre-edge values and preserves every transfer.
"""
from __future__ import annotations

import hashlib
import json
from pathlib import Path
import re

DEV = 'testbench.tb.pad.dut.u_venus_gc0802_devctrl'
DFE = 'testbench.tb.pad.dut.u_venus_gc0802_dfe_wrapper'
RTL_FILE = 'hardware/soc_hierachy/venus_gc0802_ccm_devctrl/venus_gc0802_devctrl.sv'
BACKEND_SHA = 'a9e3ac252b4de068448af145598d12bd32813272cc1829e0c1856c62b08db6b5'
RTL_COMMIT = 'a34a99aeb9245e01178d7a357c226e87a4249aef'
PROPOSAL_SHA = '25581239750d95e96daf1fb5c0c2f2092ec2b7f3fbf110f32db6a40453918b40'
EXPECTED_WRITES = [[0x1fff6154,3],[0x1fff6158,4194243]]
SIGNALS = {name:(DEV+'.'+name,width) for name,width in dict(pclk_i=1,presetn_i=1,psel_i1=1,
    biu_wr_en=1,biu_reg_addr=30,regen_mbist_enable_done2=1,regwr_mbist_enable_done=32,
    regwr_mbist_enable_done2=32,apb_req_i=66).items()}
SIGNALS.update(dfe_select=(DFE+'.psel_i1',1),dfe_response=(DFE+'.apb_resp_o',32))
GUARDS = ('psel_i1','biu_wr_en','regwr_mbist_enable_done','regwr_mbist_enable_done2')
WORDS = GUARDS[2:]


class Rejected(ValueError):
    pass


def require(ok, reason):
    if not ok:
        raise Rejected(reason)


def sha256(path):
    h = hashlib.sha256()
    with Path(path).open('rb') as stream:
        for block in iter(lambda:stream.read(1024*1024),b''):
            h.update(block)
    return h.hexdigest()


def checked(path, digest):
    require(isinstance(digest,str) and re.fullmatch('[0-9a-f]{64}',digest), 'invalid digest')
    require(sha256(path)==digest, 'input identity mismatch: '+str(path))
    return Path(path)


def integer(bits):
    require(bits is not None and re.fullmatch('[01]+',bits), 'unknown/uninitialized active field')
    return int(bits,2)


def batches(stream):
    """Yield all transitions, including repeated changes at the same time."""
    scopes, names, codes = [], set(), {}
    wanted = {path:(name,width) for name,(path,width) in SIGNALS.items()}
    directive, timescale = '', None
    for raw in stream:
        line = raw.strip()
        if not line:
            continue
        directive = (directive+' '+line).strip()
        if not directive.endswith('$end'):
            continue
        parts = directive.split()
        if parts[0] == '$scope':
            scopes.append(parts[2])
        elif parts[0] == '$upscope':
            require(bool(scopes),'unpaired scope')
            scopes.pop()
        elif parts[0] == '$timescale':
            require(timescale is None,'duplicate timescale')
            timescale = ''.join(parts[1:-1])
        elif parts[0] == '$var':
            require(len(parts)>=6,'malformed variable')
            path = '.'.join(scopes+[parts[4]])
            if path in wanted:
                name,width = wanted[path]
                require(name not in names and parts[3] not in codes and int(parts[2])==width,
                        'ambiguous/wrong-width signal: '+path)
                names.add(name)
                codes[parts[3]] = (name,width)
        elif parts[0] == '$enddefinitions':
            break
        directive = ''
    else:
        raise Rejected('missing VCD definitions')
    require(names==set(SIGNALS) and timescale=='1ps','missing signals/wrong timescale')
    values, changes, tick = {}, [], None
    before = {}
    comment = False
    for raw in stream:
        line = raw.strip()
        if not line:
            continue
        if comment:
            comment = '$end' not in line
            continue
        if line.startswith('$comment'):
            comment = '$end' not in line
            continue
        if line in ('$dumpvars','$end'):
            continue
        require(not line.startswith('$'),'unsupported/gapped VCD directive: '+line)
        if line.startswith('#'):
            now = int(line[1:])
            require(now>=0 and (tick is None or now>=tick),'reversed time')
            if tick is None or now>tick:
                if tick is not None:
                    yield tick,before,dict(values),changes
                tick, before, changes = now,dict(values),[]
            continue
        require(tick is not None,'values without timestamp')
        if line[0] in '01xXzZ':
            value,code = line[0].lower(),line[1:].strip()
        elif line[0] in 'bB':
            fields = line[1:].split()
            require(len(fields)==2,'malformed vector')
            value,code = fields[0].lower(),fields[1]
        else:
            raise Rejected('unsupported VCD value')
        if code not in codes:
            continue
        name,width = codes[code]
        require(bool(value) and len(value)<=width and re.fullmatch('[01xz]+',value),'invalid VCD bits')
        value = value.rjust(width,value[0] if value[0] in 'xz' else '0')
        previous = values.get(name)
        if previous != value:
            changes.append((name,previous,value))
            values[name] = value
    require(not comment and tick is not None,'incomplete VCD')
    yield tick,before,dict(values),changes


def request(values):
    bits = values['apb_req_i']
    address = integer(bits[:32])
    enable,write = integer(bits[32]),integer(bits[-1])
    data = integer(bits[33:65]) if write else None
    return address,enable,write,data


def inspect(stream):
    """Collect all-event guards plus independent pre-edge APB transactions."""
    reset_seen, released, last_clock, period = False,False,None,None
    dfe_started, clock_period_changes = False,[]
    pending, transactions, edges, guard_changes = None,[],[],[]
    clock_count, transition_count, end = 0,0,None
    for tick,before,after,changes in batches(stream):
        end = tick
        transition_count += len(changes)
        reset = after.get('presetn_i')
        newly_released = not released and reset=='1'
        if reset == '0':
            require(not released,'reset reasserted during observation')
            reset_seen = True
        if not released and reset == '1':
            require(reset_seen and set(after)==set(SIGNALS),'reset/initial coverage missing')
            released = True
        if not released:
            continue
        require(reset=='1','unknown reset after release')
        # Firmware explicitly exercises PLL and APB dividers during boot.
        # Keep those periods; require the qualified 19998ps period continuously
        # from the first DFE selection through native completion, not at boot.
        dfe_started = dfe_started or integer(after.get('dfe_select'))==1
        for name in WORDS:
            require(integer(after.get(name))==0,'MBIST state not zero')
        for name,old,new in changes:
            if name=='presetn_i':
                require(newly_released and old=='0' and new=='1','reset transient after release')
            if name in WORDS:
                require(integer(new)==0,'MBIST transient write/state change')
            if name in GUARDS or name=='presetn_i':
                require(new is not None and all(x in '01' for x in new),'unknown guard transition')
                guard_changes.append(dict(time_ps=tick,signal=name,before=old,after=new))
        clock_changes = [new for name,old,new in changes if name=='pclk_i']
        require(len(clock_changes)<=1,'same-timestamp clock pulse')
        require(after['pclk_i'] in ('0','1'),'unknown clock')
        if before.get('pclk_i')=='0' and after['pclk_i']=='1':
            clock_count += 1
            if last_clock is not None:
                measured = tick-last_clock
                if measured!=period:
                    clock_period_changes.append(dict(time_ps=tick,period_ps=measured))
                period = measured
                require(measured>0 and (not dfe_started or measured==19998),
                        'clock gap/period change during DFE observation')
            previous_clock,last_clock = last_clock,tick
            if before.get('presetn_i')=='1':
                select = integer(before.get('dfe_select'))
                if select:
                    address,enable,write,data = request(before)
                    require(0x1fff6000<=address<0x1fff7000 and address%4==0,'invalid DFE address')
                    key = address,write,data
                    if not enable:
                        require(pending is None,'duplicate APB SETUP')
                        pending = dict(key=key,setup_time_ps=tick,begin_time_ps=previous_clock,
                            guard_at_setup={name:before[name] for name in GUARDS})
                    else:
                        require(pending is not None and pending['key']==key,'missing/changed APB SETUP')
                        transactions.append(dict(index=len(transactions),address=address,write=write,
                            value=data if write else integer(before['dfe_response']),
                            setup_time_ps=pending['setup_time_ps'],begin_time_ps=pending['begin_time_ps'],
                            time_ps=tick,guard_at_setup=pending['guard_at_setup']))
                        pending = None
                else:
                    require(pending is None,'APB deselected before ACCESS')
        decoder = [(old,new) for name,old,new in changes if name=='regen_mbist_enable_done2']
        if decoder:
            require(len(decoder)==1,'transient/duplicate decoder edge')
            old,new = decoder[0]
            require(old in ('0','1') and new in ('0','1'),'unknown decoder edge')
            require(integer(after['dfe_select'])==1,'decoder edge without selected DFE')
            address,enable,write,data = request(after)
            require(enable==0 and write==1,'decoder edge not a DFE write SETUP')
            require(integer(after['biu_reg_addr'])==address>>2,'address bus mismatch')
            require(integer(new)==int(((address>>2)&511)==0x55),'decoder address mismatch')
            for name in GUARDS:
                require(integer(before.get(name))==integer(after[name])==0,'DEVCTRL active at decoder edge')
            edges.append(dict(time_ps=tick,address=address,value=data,old=int(old),new=int(new)))
    require(released and pending is None and clock_count>2,'incomplete reset/APB/clock coverage')
    return dict(end_time_ps=end,clock_period_ps=period,clock_samples=clock_count,
        clock_period_changes=clock_period_changes,
        signal_transitions=transition_count,transactions=transactions,decoder_edges=edges,
        guard_changes=guard_changes,all_mbist_state_transitions_zero=True)


def judge(trace, log, expected_writes):
    """Scope-limited classification, not numerical or native-run acceptance."""
    require('Test complete!' in log and 'GPIO4 pluse' not in log,'missing completion or self-check failure')
    require(not re.search(r'Error-\[|Fatal:|Objects .*not found|\$stop',log),'tool/capture failure')
    finish = re.findall(r'^Time: (\d+) ps$',log,re.M)
    require(len(finish)==1 and trace['end_time_ps']==int(finish[0]),'waveform does not cover complete native run')
    lines, errors = log.splitlines(),[]
    regex = r'^Error: "([^"\n]+)", 259: '+re.escape(DEV)+r': at time (\d+) ps$'
    for index,line in enumerate(lines):
        if not line.startswith('Error:'):
            continue
        match = re.fullmatch(regex,line)
        require(match is not None and match[1].endswith('/'+RTL_FILE),'unexpected simulator error/source')
        require(index+1<len(lines) and lines[index+1]=='regen_mbist_enable_done2 toggle unexpectedly',
                'unexpected diagnostic message')
        errors.append(dict(time_ps=int(match[2]),line=index+1,source=match[1]))
    require(sum(e['time_ps']==0 for e in errors)==1,'time-zero diagnostic coverage differs')
    active = [e for e in errors if e['time_ps']]
    edges = trace['decoder_edges']
    require(len(active)==len(edges)==len(expected_writes)==2,'missing/extra diagnostic or decoder edge')
    require([e['time_ps'] for e in active]==[e['time_ps'] for e in edges],'error/edge timestamps differ')
    require([(e['address'],e['value']) for e in edges]==[tuple(x) for x in expected_writes],
            'unexpected DFE address/value sequence')
    require([e['new'] for e in edges]==[1,0],'unexpected edge direction')
    intervals = []
    for edge,error in zip(edges,active):
        matches = [op for op in trace['transactions'] if op['begin_time_ps']==edge['time_ps']
                   and op['address']==edge['address'] and op['value']==edge['value'] and op['write']==1]
        require(len(matches)==1,'diagnostic has no unique complete APB transaction')
        op, = matches
        require(op['setup_time_ps']-op['begin_time_ps']==trace['clock_period_ps']
                and op['time_ps']-op['setup_time_ps']==trace['clock_period_ps'],'APB timing mismatch')
        require(all(integer(v)==0 for v in op['guard_at_setup'].values()),'DEVCTRL active at SETUP')
        # Check every transition, not only final values, through ACCESS completion.
        events = [g for g in trace['guard_changes'] if edge['time_ps']<=g['time_ps']<=op['time_ps']]
        require(all(g['signal']!='presetn_i' and integer(g['after'])==0
                    and integer(g['before'])==0 for g in events),'transient DEVCTRL activity in access window')
        intervals.append(dict(error=error,operation=op,guard_transition_count=len(events)))
    # Address-only decode can remain asserted while the bus is idle between
    # the two writes. Protect that interval too, including writes of zero.
    span = [g for g in trace['guard_changes']
            if edges[0]['time_ps']<=g['time_ps']<=intervals[-1]['operation']['time_ps']]
    require(all(g['signal']!='presetn_i' and integer(g['before'])==integer(g['after'])==0 for g in span),
            'DEVCTRL activity while MBIST address decode is held')
    return dict(status='PASS_DIAGNOSTIC_CLASSIFICATION_ONLY',classified_errors=intervals,
        time_zero_errors=[e for e in errors if not e['time_ps']],
        native_result_unchanged=True,all_output_comparison_status='NOT_RUN',application_complete=False,
        scope='Only the frozen two-write TX setup and its unselected address-only diagnostic',
        full_transition_check=True,complete_native_time_coverage=True,
        continuous_guard_interval_ps=[edges[0]['time_ps'],intervals[-1]['operation']['time_ps']])


def classify_native_run(*, enabled=False, request_path=None, request_sha256=None):
    """Independent explicit opt-in entry. It never edits a native report."""
    if not enabled:
        return dict(status='DISABLED',native_result_unchanged=True,application_complete=False)
    request_path = checked(request_path,request_sha256)
    doc = json.loads(request_path.read_text())
    require(doc['schema']=='venus1-mbist-classification-request/v1','request schema mismatch')
    paths = {key:checked(ref['path'],ref['sha256']) for key,ref in doc['inputs'].items()}
    native = json.loads(paths['native'].read_text())
    require(native['backend_id']=='venus1p0-64x512-300mhz' and native['fresh_compile']
            and native['source_unchanged'],'backend/fresh/identity mismatch')
    require(native['source_identity_before']['head']==RTL_COMMIT,'RTL commit mismatch')
    require(native['source_identity_before']==native['source_identity_after'],'native source identity differs')
    require(sha256(paths['backend'])==BACKEND_SHA,'backend manifest mismatch')
    require(len(native['cases'])==1,'one case required')
    case, = native['cases']
    require(case['l1_sha256']==sha256(paths['firmware']),'firmware mismatch')
    require(case['test_complete_seen'] and not case['crc_failure_seen'],'native self-check failure')
    require(case['non_axi_simulator_error_count']==2 and case['axi_unknown_assertion_count']==0,
            'unexpected native errors')
    require(native['status']=='FAIL' and case['status']=='FAIL','original native FAIL required')
    for key in ('vpd','log'):
        matches = [e for e in case['evidence'] if Path(e['path']).resolve()==paths[key].resolve()]
        require(len(matches)==1 and matches[0]['sha256']==sha256(paths[key]),'native evidence mismatch')
    conversion = json.loads(paths['conversion'].read_text())
    require(conversion['returncode']==0 and not conversion['dry_run'],'VCD conversion incomplete')
    require([str(paths['vpd']),str(paths['vcd'])]==conversion['argv'][-2:],'converter input/output differs')
    require(conversion['argv'][:2]==[str(paths['converter']),'-full64'],'unqualified conversion argv')
    contract = json.loads(paths['contract'].read_text())
    require(contract['schema']=='venus1-mbist-diagnostic-contract/v1','contract schema mismatch')
    require(contract['backend_sha256']==BACKEND_SHA and contract['rtl_commit']==RTL_COMMIT,'contract backend mismatch')
    require(contract['signal_contract']=={k:list(v) for k,v in SIGNALS.items()},'signal contract mismatch')
    require(contract['classifier_sha256']==sha256(Path(__file__)),'classifier digest mismatch')
    require(contract['expected_dfe_writes']==EXPECTED_WRITES,'unqualified DFE sequence contract')
    require(sha256(paths['approved_proposal'])==PROPOSAL_SHA,'unapproved classification proposal')
    approved = json.loads(paths['approved_proposal'].read_text())
    required = {str(Path(p).relative_to(Path(doc['rtl_root']))):digest
                for p,digest in approved['evidence'].items()
                if str(p).startswith(str(Path(doc['rtl_root']))+'/')}
    require(bool(required) and all(contract['source_digests'].get(p)==d for p,d in required.items()),
            'required immutable RTL source binding missing or changed')
    for relative,digest in contract['source_digests'].items():
        p = Path(relative)
        require(not p.is_absolute() and '..' not in p.parts,'unsafe source path')
        checked(Path(doc['rtl_root'])/p,digest)
        checked(Path(doc['rtl_snapshot'])/p,digest)
    launch = json.loads(paths['launch'].read_text())
    require(launch['inputs_unchanged'] and not launch.get('missing_signals'),'launch identity/capture failure')
    ucli = paths['ucli'].read_text()
    require(ucli.count('dump -file ')==1 and ucli.count('dump -deltaCycle on -fid $axi_fid')==1,
            'unqualified delta-preserving dump configuration')
    require(all(path[len('testbench.'):] in ucli for path,width in SIGNALS.values()),'incomplete UCLI signal set')
    require(not re.search(r'\b(force|deposit|release|call|stop)\b',ucli),'mutating/unsupported UCLI command')
    with paths['vcd'].open() as stream:
        trace = inspect(stream)
    report = judge(trace,paths['log'].read_text(),contract['expected_dfe_writes'])
    require(all(sha256(paths[k])==ref['sha256'] for k,ref in doc['inputs'].items()),'input changed during classification')
    for relative,digest in contract['source_digests'].items():
        checked(Path(doc['rtl_root'])/relative,digest)
        checked(Path(doc['rtl_snapshot'])/relative,digest)
    require(sha256(request_path)==request_sha256,'request changed')
    report.update(native_status=native['status'],request=dict(path=str(request_path),sha256=request_sha256),
        end_time_ps=trace['end_time_ps'],clock_samples=trace['clock_samples'],
        signal_transitions=trace['signal_transitions'],apb_transactions=len(trace['transactions']))
    return report
