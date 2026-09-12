from pathlib import Path
import json
import tempfile
import unittest
import os
import sys
import time
import subprocess
import signal

from ace_echo.process import Runner
from ace_echo.process import CommandFailed


class RunnerTests(unittest.TestCase):
    @unittest.skipUnless(os.name == 'posix', 'Linux execution host')
    def test_keyboard_interrupt_cleans_group_and_records_cancellation(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory)
            source=str(Path(__file__).resolve().parents[1]/'src')
            child="import pathlib,time; pathlib.Path('ready').write_text('yes'); time.sleep(60)"
            worker_code=("import sys; sys.path.insert(0,"+repr(source)+"); from pathlib import Path; from ace_echo.process import Runner; "
                "Runner(Path('commands')).run('cancel',[sys.executable,'-c',"+repr(child)+"],cwd=Path.cwd(),timeout=30)")
            worker=subprocess.Popen([sys.executable,'-c',worker_code],cwd=root,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
            try:
                deadline=time.monotonic()+5
                while not (root/'ready').exists() and time.monotonic()<deadline: time.sleep(.02)
                self.assertTrue((root/'ready').exists())
                worker.send_signal(signal.SIGINT)
                worker.wait(timeout=5)
                receipt=json.loads((root/'commands/01-cancel.json').read_text())
                self.assertEqual(receipt['termination_reason'],'CANCELLED')
                self.assertEqual(receipt['returncode'],130)
                self.assertNotEqual(receipt['cleanup'],'NOT_NEEDED')
            finally:
                if worker.poll() is None:
                    worker.send_signal(signal.SIGINT)
                    worker.wait(timeout=5)

    @unittest.skipUnless(os.name == 'posix', 'Linux execution host')
    def test_normal_execution_and_launch_failure_are_recorded(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory)
            runner=Runner(root/'commands')
            self.assertEqual(runner.run('ok',[sys.executable,'-c','print(42)'],cwd=root,timeout=3).returncode,0)
            result=runner.run('missing',['/no-such-ace-echo-tool'],cwd=root,timeout=3,check=False)
            self.assertEqual(result.termination_reason,'LAUNCH_ERROR')

    @unittest.skipUnless(os.name == 'posix', 'Linux execution host')
    def test_timeout_stops_sigterm_ignoring_grandchild(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory)
            code="import signal,time,pathlib; signal.signal(signal.SIGTERM,signal.SIG_IGN); p=pathlib.Path('heartbeat'); "
            code += "\nwhile True: p.write_text(str(time.monotonic())); time.sleep(.02)"
            parent="import subprocess,sys,time; subprocess.Popen([sys.executable,'-c',"+repr(code)+"]); time.sleep(60)"
            with self.assertRaises(CommandFailed) as error:
                Runner(root/'commands').run('tree',[sys.executable,'-c',parent],cwd=root,timeout=1)
            self.assertEqual(error.exception.result.termination_reason,'TIMEOUT')
            self.assertEqual(error.exception.result.returncode,124)
            before=(root/'heartbeat').read_text()
            time.sleep(.15)
            self.assertEqual((root/'heartbeat').read_text(),before)

    @unittest.skipUnless(os.name == 'posix', 'Linux execution host')
    def test_parent_success_with_live_child_is_not_success(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory)
            parent="import subprocess,sys; subprocess.Popen([sys.executable,'-c','import time; time.sleep(60)'])"
            result=Runner(root/'commands').run('orphan',[sys.executable,'-c',parent],cwd=root,timeout=3,check=False)
            self.assertEqual(result.returncode,125)
            self.assertEqual(result.termination_reason,'ORPHANED_CHILDREN')

    def test_dry_run_records_without_execution(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            runner = Runner(root / "commands", dry_run=True)
            result = runner.run(
                "probe", ["definitely-not-a-command", "arg with space"],
                cwd=root, timeout=1,
            )
            self.assertTrue(result.dry_run)
            self.assertIsNone(result.returncode)
            record = json.loads((root / "commands/01-probe.json").read_text())
            self.assertEqual(record["argv"][1], "arg with space")


if __name__ == "__main__":
    unittest.main()
