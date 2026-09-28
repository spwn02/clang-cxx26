#!/usr/bin/env python3
"""Persist a serial Luna work queue; run under a systemd user service.

State and JSONL logs belong outside the checkout. Successful implementation
turns await integration review; they never imply issue completion. Non-limit
failures stop the queue for inspection instead of repeatedly spending usage.
"""
import argparse
import datetime as dt
import fcntl
import json
import os
from pathlib import Path
import re
import subprocess
import time


def save(path, state):
    temporary = path.with_suffix('.tmp')
    with temporary.open('w') as stream:
        json.dump(state, stream, indent=2)
        stream.write('\n')
        stream.flush()
        os.fsync(stream.fileno())
    temporary.replace(path)
    directory = os.open(path.parent, os.O_RDONLY | os.O_DIRECTORY)
    try:
        os.fsync(directory)
    finally:
        os.close(directory)


def limit_delay(message, now):
    """Only explicit usage-limit errors trigger retry, never worker prose."""
    if not re.search(r'usage[_ -]limit|usage limit|hit your .*limit', message, re.I):
        return None
    match = re.search(r'(?:resets?_at|retry_at)[\s\"\:]+(\d{10})\b', message)
    if match:
        return max(60, int(match[1]) - now)
    match = re.search(r'\d{4}-\d\d-\d\dT\d\d:\d\d(?::\d\d)?(?:Z|[+-]\d\d:\d\d)', message)
    if match:
        reset = dt.datetime.fromisoformat(match[0].replace('Z', '+00:00')).timestamp()
        return max(60, reset - now)
    match = re.search(r'(?:try again|retry|resets?) in\s+(\d+)\s*(seconds?|minutes?|hours?)', message, re.I)
    if match:
        return max(60, int(match[1]) * {'s': 1, 'm': 60, 'h': 3600}[match[2][0].lower()])
    match = re.search(r'(?:try again|retry|resets?) at\s+(.+)', message, re.I)
    if match:
        reported = re.sub(r'(\d)(st|nd|rd|th)\b', r'\1', match[1], flags=re.I)
        reported = reported.split('"')[0].strip().rstrip('.')
        local_now = dt.datetime.fromtimestamp(now)
        for pattern in ('%b %d, %Y %I:%M %p', '%B %d, %Y %I:%M %p',
                        '%I:%M %p on %b %d, %Y', '%Y-%m-%d %H:%M',
                        '%I:%M %p', '%H:%M'):
            try:
                reset = dt.datetime.strptime(reported, pattern)
            except ValueError:
                continue
            if pattern in ('%I:%M %p', '%H:%M'):
                reset = reset.replace(year=local_now.year, month=local_now.month, day=local_now.day)
                if reset.timestamp() <= now:
                    reset += dt.timedelta(days=1)
            return max(60, reset.timestamp() - now)
    return 5 * 3600


def run(state_path, executable):
    with state_path.with_suffix('.lock').open('w') as lock:
        fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
        state = json.loads(state_path.read_text())
        state['supervisor_pid'] = os.getpid()
        # systemd KillMode=control-group removes the old worker before restart.
        for task in state['tasks']:
            if task['status'] == 'running':
                task['status'] = 'pending'
                task['recovered_at'] = time.time()
        save(state_path, state)
        while True:
            if (state_path.parent / 'STOP').exists():
                return
            task = next((t for t in state['tasks'] if t['status'] in ('pending', 'waiting_limit')), None)
            if task is None:
                return
            if any(t['status'] == 'infrastructure_failure' for t in state['tasks']):
                return
            if task.get('retry_at', 0) > time.time():
                time.sleep(min(30, task['retry_at'] - time.time()))
                continue
            task['attempts'] = task.get('attempts', 0) + 1
            log = state_path.parent / f"{task['name']}-{task['attempts']}.jsonl"
            error_log = log.with_suffix('.stderr')
            task.update(status='running', started_at=time.time(), log=str(log))
            save(state_path, state)
            args = [executable, 'exec']
            if task.get('session_id'):
                args += ['resume', task['session_id']]
            else:
                args += ['--approve-for-me']
            args += ['-m', 'gpt-6-luna', '-c', 'model_reasoning_effort="low"', '--json', '-']
            prompt = task['prompt']
            if task.get('session_id'):
                prompt = ('Resume: first inspect git status, saved changes and prior logs. '
                          'Continue the authorized task, preserving existing work.\n' + prompt)
            errors = []
            completed = False
            with error_log.open('w') as stderr, log.open('a') as output:
                try:
                    process = subprocess.Popen(args, cwd=task['cwd'], stdin=subprocess.PIPE,
                                               stdout=subprocess.PIPE, stderr=stderr, text=True)
                except OSError as error:
                    task.update(status='infrastructure_failure', error=str(error), finished_at=time.time())
                    save(state_path, state)
                    return
                task['worker_pid'] = process.pid
                save(state_path, state)
                try:
                    process.stdin.write(prompt)
                    process.stdin.close()
                except BrokenPipeError:
                    # CLI argument/auth failures can exit before consuming stdin.
                    pass
                for line in process.stdout:
                    output.write(line)
                    output.flush()
                    try:
                        event = json.loads(line)
                    except json.JSONDecodeError:
                        continue
                    if event.get('type') == 'thread.started':
                        task['session_id'] = event['thread_id']
                        save(state_path, state)
                    if event.get('type') in ('error', 'turn.failed'):
                        errors.append(json.dumps(event))
                    if event.get('type') == 'turn.completed':
                        completed = True
                rc = process.wait()
            task.update(exit_code=rc, finished_at=time.time())
            task.pop('worker_pid', None)
            message = '\n'.join(errors)
            delay = limit_delay(message, time.time())
            if delay is not None:
                task.update(status='waiting_limit', retry_at=time.time() + delay, error=message)
                save(state_path, state)
                continue
            task['status'] = 'review_required' if completed and rc == 0 else 'infrastructure_failure'
            if task['status'] == 'review_required' and task.get('result_path'):
                result_path = Path(task['result_path'])
                try:
                    result = json.loads(result_path.read_text())
                    if result_path.stat().st_mtime < task['started_at']:
                        raise ValueError('stale integration result')
                    if result.get('complete') is True:
                        task['status'] = 'reported_complete'
                    elif result.get('continue') is True:
                        task.update(status='pending', retry_at=time.time() + 60)
                    else:
                        task['status'] = 'needs_attention'
                    task['result'] = result
                except (OSError, ValueError) as error:
                    task.update(status='needs_attention', error=str(error))
            if task['status'] == 'infrastructure_failure':
                task['error'] = message or error_log.read_text()[-4000:]
            save(state_path, state)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('state', type=Path)
    parser.add_argument('--codex', default='codex')
    args = parser.parse_args()
    run(args.state.resolve(), args.codex)


if __name__ == '__main__':
    main()
