"""Run a command in a detached background process and record its outcome in files.

Use this instead of writing throwaway launcher scripts: the agent's interactive
console can drop output or be interrupted, but a detached run keeps going and
leaves its result on disk.

Writes (all in --out-dir, default %TEMP%):
  <tag>.log        combined stdout + stderr of the command
  <tag>.done.json  {"exit": <code>, "seconds": <elapsed>, "command": [...]} once finished

Usage:
  py scripts/run_detached.py --tag NAME [--out-dir DIR] [--cwd DIR] -- <command> [args...]
  py scripts/run_detached.py --wait NAME [--out-dir DIR] [--timeout SECONDS]
"""

import argparse
import json
import os
import subprocess
import sys
import tempfile
import time

detached_flags = subprocess.DETACHED_PROCESS | subprocess.CREATE_NEW_PROCESS_GROUP | subprocess.CREATE_NO_WINDOW


def paths_for(out_dir, tag):
    return os.path.join(out_dir, f"{tag}.log"), os.path.join(out_dir, f"{tag}.done.json")


def launch(arguments):
    log_path, done_path = paths_for(arguments.out_dir, arguments.tag)
    if os.path.exists(done_path):
        os.remove(done_path)
    child = [sys.executable, os.path.abspath(__file__), "--child", arguments.tag,
             "--out-dir", arguments.out_dir, "--cwd", arguments.cwd or os.getcwd(), "--", *arguments.command]
    subprocess.Popen(child, creationflags=detached_flags, close_fds=True,
                     stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, stdin=subprocess.DEVNULL)
    print(f"launched {arguments.tag}: log={log_path}")
    return 0


def run_child(arguments):
    log_path, done_path = paths_for(arguments.out_dir, arguments.child)
    started = time.time()
    with open(log_path, "w", encoding="utf-8", errors="replace") as log:
        # CREATE_NO_WINDOW: a console app started from this console-less process would
        # otherwise get its own visible terminal window, which steals focus from the app
        # under test and kills the command if that window closes.
        exit_code = subprocess.run(arguments.command, cwd=arguments.cwd, stdout=log, stderr=subprocess.STDOUT,
                                   stdin=subprocess.DEVNULL, creationflags=subprocess.CREATE_NO_WINDOW).returncode
    with open(done_path, "w", encoding="utf-8") as handle:
        json.dump({"exit": exit_code, "seconds": round(time.time() - started, 1), "command": arguments.command},
                  handle, indent=2)
    return 0


def wait(arguments):
    _, done_path = paths_for(arguments.out_dir, arguments.wait)
    deadline = time.time() + arguments.timeout
    while time.time() < deadline:
        if os.path.exists(done_path):
            with open(done_path, "r", encoding="utf-8") as handle:
                print(handle.read())
            return 0
        time.sleep(3)
    print(f"timeout: {arguments.wait} not finished after {arguments.timeout}s")
    return 1


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--tag")
    parser.add_argument("--wait")
    parser.add_argument("--child", help=argparse.SUPPRESS)
    parser.add_argument("--out-dir", default=tempfile.gettempdir())
    parser.add_argument("--cwd")
    parser.add_argument("--timeout", type=int, default=600)
    parser.add_argument("command", nargs=argparse.REMAINDER)
    arguments = parser.parse_args()
    if arguments.command and arguments.command[0] == "--":
        arguments.command = arguments.command[1:]

    if arguments.child:
        return run_child(arguments)
    if arguments.wait:
        return wait(arguments)
    if arguments.tag and arguments.command:
        return launch(arguments)
    parser.print_usage()
    return 2


if __name__ == "__main__":
    sys.exit(main())
