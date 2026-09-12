import copy
from io import StringIO
import unittest
from unittest.mock import patch
from ace_echo import venus1_mbist_classifier as m

P = 19998
EXPECTED = [(0x1fff6154,3),(0x1fff6158,4194243)]


def wave(extra=None,omit=None,width_delta=0):
    names = [n for n in m.SIGNALS if n!=omit]
    codes = {n:'v%d'%i for i,n in enumerate(names)}
    header = ['$timescale 1ps $end']
    for name in names:
        path,width = m.SIGNALS[name]
        parts = path.split('.')
        header.extend('$scope module '+x+' $end' for x in parts[:-1])
        header.append('$var reg %d %s %s $end'%(width+(width_delta if name=='psel_i1' else 0),codes[name],parts[-1]))
        header.extend('$upscope $end' for x in parts[:-1])
    header.append('$enddefinitions $end')
    events = [(0,[(n,0) for n in names]),(1,[('presetn_i',1)])]
    for i in range(9):
        tick=10+i*P
        change=[('pclk_i',1)]
        if i in (2,5):
            address,value=EXPECTED[int(i==5)]
            change += [('apb_req_i',(address<<34)|(value<<1)|1),('biu_reg_addr',address>>2),
                ('regen_mbist_enable_done2',int(i==2)),('dfe_select',1)]
        if i in (3,6):
            address,value=EXPECTED[int(i==6)]
            change.append(('apb_req_i',(address<<34)|(1<<33)|(value<<1)|1))
        if i in (4,7):
            address,value=EXPECTED[int(i==7)]
            change += [('dfe_select',0),('apb_req_i',(address<<34)|(value<<1)|1)]
        events.append((tick,change))
        if i!=8:
            events.append((tick+P//2,[('pclk_i',0)]))
    if extra:
        events.extend(extra)
    body=[]
    for tick,changes in sorted(events,key=lambda x:x[0]):
        body.append('#'+str(tick))
        for name,value in changes:
            if name not in codes:
                continue
            bits = value if isinstance(value,str) else format(value,'b')
            body.append('b'+bits+' '+codes[name])
    return '\n'.join(header+body)+'\n'


def log():
    rows=[]
    for tick in (0,10+2*P,10+5*P):
        rows.extend(['Error: "/snapshot/'+m.RTL_FILE+'", 259: '+m.DEV+': at time %d ps'%tick,
                     'regen_mbist_enable_done2 toggle unexpectedly'])
    return '\n'.join(rows+['Test complete!','Time: %d ps'%(10+8*P)])+'\n'


class ClassifierTests(unittest.TestCase):
    def run_wave(self,text=None,logtext=None):
        return m.judge(m.inspect(StringIO(wave() if text is None else text)),
                       log() if logtext is None else logtext,EXPECTED)

    def test_complete_positive(self):
        r=self.run_wave()
        self.assertEqual(r['status'],'PASS_DIAGNOSTIC_CLASSIFICATION_ONLY')
        self.assertFalse(r['application_complete'])
        self.assertEqual(r['all_output_comparison_status'],'NOT_RUN')

    def test_disabled_does_not_read_any_input(self):
        with patch.object(m,'checked',side_effect=AssertionError('unexpected read')):
            self.assertEqual(m.classify_native_run()['status'],'DISABLED')

    def test_psel_same_timestamp_pulse_rejected(self):
        with self.assertRaisesRegex(m.Rejected,'transient DEVCTRL'):
            self.run_wave(wave([(57000,[('psel_i1',1),('psel_i1',0)])]))

    def test_write_same_timestamp_pulse_rejected(self):
        with self.assertRaisesRegex(m.Rejected,'transient DEVCTRL'):
            self.run_wave(wave([(57000,[('biu_wr_en',1),('biu_wr_en',0)])]))

    def test_repeated_timestamp_not_collapsed(self):
        with self.assertRaises(m.Rejected):
            self.run_wave(wave([(57000,[('psel_i1',1)]),(57000,[('psel_i1',0)])]))

    def test_mbist_word0_transient_rejected(self):
        with self.assertRaises(m.Rejected):
            self.run_wave(wave([(57000,[('regwr_mbist_enable_done',1),('regwr_mbist_enable_done',0)])]))

    def test_mbist_word1_transient_rejected(self):
        with self.assertRaises(m.Rejected):
            self.run_wave(wave([(57000,[('regwr_mbist_enable_done2',1),('regwr_mbist_enable_done2',0)])]))

    def test_mbist_change_outside_error_window_rejected(self):
        with self.assertRaises(m.Rejected):
            self.run_wave(wave([(150000,[('regwr_mbist_enable_done2',1),('regwr_mbist_enable_done2',0)])]))

    def test_devctrl_pulse_between_error_accesses_rejected(self):
        with self.assertRaises(m.Rejected):
            self.run_wave(wave([(90000,[('psel_i1',1),('psel_i1',0)])]))

    def test_zero_mbist_write_between_accesses_rejected(self):
        with self.assertRaises(m.Rejected):
            self.run_wave(wave([(90000,[('biu_wr_en',1),('biu_wr_en',0)])]))

    def test_reset_pulse_after_accesses_rejected(self):
        with self.assertRaises(m.Rejected):
            self.run_wave(wave([(150000,[('presetn_i',0),('presetn_i',1)])]))

    def test_selected_devctrl_at_edge_rejected(self):
        with self.assertRaises(m.Rejected):
            self.run_wave(wave([(40006,[('psel_i1',1)])]))

    def test_unknown_guard_even_transient_rejected(self):
        for name in m.GUARDS:
            with self.subTest(name=name),self.assertRaises(m.Rejected):
                self.run_wave(wave([(57000,[(name,'x'),(name,0)])]))

    def test_reset_pulse_rejected(self):
        with self.assertRaises(m.Rejected):
            self.run_wave(wave([(57000,[('presetn_i',0),('presetn_i',1)])]))

    def test_dumpoff_gap_rejected(self):
        with self.assertRaises(m.Rejected):
            self.run_wave(wave().replace('#60004','$dumpoff\n$end\n#60004'))

    def test_truncated_trace_rejected(self):
        with self.assertRaises(m.Rejected):
            self.run_wave(wave().split('#159994')[0])

    def test_missing_signal_rejected(self):
        with self.assertRaises(m.Rejected):
            self.run_wave(wave(omit='biu_wr_en'))

    def test_wrong_signal_width_rejected(self):
        with self.assertRaises(m.Rejected):
            self.run_wave(wave(width_delta=1))

    def test_wrong_timescale_rejected(self):
        with self.assertRaises(m.Rejected):
            self.run_wave(wave().replace('1ps','1ns'))

    def test_reversed_timestamp_rejected(self):
        with self.assertRaises(m.Rejected):
            self.run_wave(wave()+'#1\n')

    def test_clock_same_timestamp_pulse_rejected(self):
        with self.assertRaises(m.Rejected):
            self.run_wave(wave([(57000,[('pclk_i',1),('pclk_i',0)])]))

    def test_unknown_clock_rejected(self):
        with self.assertRaises(m.Rejected):
            self.run_wave(wave([(57000,[('pclk_i','x')])]))

    def test_clock_period_change_in_dfe_phase_rejected(self):
        with self.assertRaises(m.Rejected):
            self.run_wave(wave().replace('#60004','#60005'))

    def test_missing_apb_setup_rejected(self):
        address,value=EXPECTED[0]
        with self.assertRaises(m.Rejected):
            self.run_wave(wave([(40006,[('apb_req_i',(address<<34)|(1<<33)|(value<<1)|1)])]))

    def test_changed_apb_value_rejected(self):
        address,value=EXPECTED[0]
        with self.assertRaises(m.Rejected):
            self.run_wave(wave([(60004,[('apb_req_i',(address<<34)|(1<<33)|(4<<1)|1)])]))

    def test_dfe_deselected_early_rejected(self):
        with self.assertRaises(m.Rejected):
            self.run_wave(wave([(60004,[('dfe_select',0)])]))

    def test_wrong_expected_value_rejected(self):
        with self.assertRaises(m.Rejected):
            m.judge(m.inspect(StringIO(wave())),log(),[(EXPECTED[0][0],4),EXPECTED[1]])

    def test_duplicate_decoder_edge_rejected(self):
        with self.assertRaises(m.Rejected):
            self.run_wave(wave([(57000,[('regen_mbist_enable_done2',0),('regen_mbist_enable_done2',1)])]))

    def test_extra_error_rejected(self):
        with self.assertRaises(m.Rejected):
            self.run_wave(logtext=log()+'Error: arbitrary functional error\n')

    def test_wrong_source_rejected(self):
        with self.assertRaises(m.Rejected):
            self.run_wave(logtext=log().replace(m.RTL_FILE,'other.sv'))

    def test_wrong_message_rejected(self):
        with self.assertRaises(m.Rejected):
            self.run_wave(logtext=log().replace('toggle unexpectedly','functional failure'))

    def test_wrong_line_rejected(self):
        with self.assertRaises(m.Rejected):
            self.run_wave(logtext=log().replace('259:','260:'))

    def test_wrong_error_time_rejected(self):
        with self.assertRaises(m.Rejected):
            self.run_wave(logtext=log().replace('40006 ps','40007 ps'))

    def test_missing_error_rejected(self):
        with self.assertRaises(m.Rejected):
            self.run_wave(logtext='\n'.join(log().splitlines()[2:]))

    def test_missing_observer_warning_rejected(self):
        with self.assertRaises(m.Rejected):
            self.run_wave(logtext=log()+'Objects foo not found. Ignored\n')

    def test_self_check_failure_rejected(self):
        with self.assertRaises(m.Rejected):
            self.run_wave(logtext=log()+'GPIO4 pluse\n')

    def test_completion_missing_rejected(self):
        with self.assertRaises(m.Rejected):
            self.run_wave(logtext=log().replace('Test complete!','timeout'))

    def test_duplicate_completion_time_rejected(self):
        with self.assertRaises(m.Rejected):
            self.run_wave(logtext=log()+'Time: 159994 ps\n')

    def test_positive_native_data_is_not_relabelled(self):
        text=log()
        self.run_wave(logtext=text)
        self.assertEqual(text,log())


if __name__=='__main__':
    unittest.main()
