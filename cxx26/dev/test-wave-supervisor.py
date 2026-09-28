#!/usr/bin/env python3
"""Exercise limit handling, durable sessions, recovery, and duplicate exclusion."""
import importlib.util
import fcntl
import json
from pathlib import Path
import subprocess
import tempfile
import unittest

SCRIPT = Path(__file__).with_name('wave-supervisor.py')
spec = importlib.util.spec_from_file_location('supervisor', SCRIPT)
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


class SupervisorTests(unittest.TestCase):
    def test_limit_reset(self):
        self.assertIsNone(module.limit_delay('network disconnected', 1000))
        self.assertEqual(module.limit_delay('usage_limit reached', 1000), 18000)
        self.assertEqual(module.limit_delay('usage limit; retry in 2 hours', 1000), 7200)
        self.assertEqual(module.limit_delay('usage limit resets_at: 2000000000', 1999999000), 1000)

    def test_recover_session_and_stop_on_failure(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            fake = root / 'codex'
            fake.write_text('#!/usr/bin/env python3\nimport sys,json\nsys.stdin.read()\n'
                            'print(json.dumps({"type":"thread.started","thread_id":"saved"}))\n'
                            'print(json.dumps({"type":"error","message":"unsupported model"}))\n'
                            'sys.exit(1)\n')
            fake.chmod(0o755)
            state = root / 'state.json'
            module.save(state, {'tasks': [{'name': 'one', 'cwd': folder,
                                         'prompt': 'test', 'status': 'running'}]})
            subprocess.run(['python3', str(SCRIPT), str(state), '--codex', str(fake)], check=True)
            task = json.loads(state.read_text())['tasks'][0]
            self.assertEqual(task['session_id'], 'saved')
            self.assertEqual(task['status'], 'infrastructure_failure')
            self.assertIn('recovered_at', task)

    def test_duplicate_runner_is_excluded(self):
        with tempfile.TemporaryDirectory() as folder:
            state = Path(folder) / 'state.json'
            module.save(state, {'tasks': []})
            with state.with_suffix('.lock').open('w') as lock:
                fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
                result = subprocess.run(['python3', str(SCRIPT), str(state)], capture_output=True)
                self.assertNotEqual(result.returncode, 0)

    def test_limit_preserves_session_and_does_not_dispatch_more(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            fake = root / 'codex'
            fake.write_text('#!/usr/bin/env python3\nimport sys,json\nfrom pathlib import Path\n'
                            'sys.stdin.read()\n'
                            'print(json.dumps({"type":"thread.started","thread_id":"saved"}))\n'
                            'print(json.dumps({"type":"error","message":"usage limit reached"}))\n'
                            'Path("STOP").touch()\nsys.exit(1)\n')
            fake.chmod(0o755)
            state = root / 'state.json'
            tasks = [{'name': n, 'cwd': folder, 'prompt': 'test', 'status': 'pending'}
                     for n in ('one', 'two')]
            module.save(state, {'tasks': tasks})
            subprocess.run(['python3', str(SCRIPT), str(state), '--codex', str(fake)], check=True)
            tasks = json.loads(state.read_text())['tasks']
            self.assertEqual(tasks[0]['session_id'], 'saved')
            self.assertEqual(tasks[0]['status'], 'waiting_limit')
            self.assertGreater(tasks[0]['retry_at'], tasks[0]['finished_at'] + 17900)
            self.assertEqual(tasks[1]['status'], 'pending')


if __name__ == '__main__':
    unittest.main()
