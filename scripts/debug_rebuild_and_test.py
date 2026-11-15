"""Clean Debug rebuild of yampt.sln, then run the Debug logic suite.

Debug builds enable MSVC run-time checks (stack corruption, uninitialized use)
that Release silently skips, so this is the stricter verification pass.

Results are written to files so they survive an unreliable console:
  <out_dir>/debug_rebuild.txt   MSBuild output
  <out_dir>/debug_tests.xml     Catch2 XML, streamed; the last <TestCase> is
                                where a crash or modal CRT dialog stalled
  <out_dir>/debug_done.json     {"build_exit", "test_exit", "stalled", "cases_ok", "cases_failed"}

Usage:
  py scripts/debug_rebuild_and_test.py [--skip-build] [--out-dir DIR] [--stall-seconds N]
"""

import argparse
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile
import time

repo_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
msbuild_path = r"C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe"
debug_dir = os.path.join(repo_root, "x64", "Debug")
test_exe = os.path.join(debug_dir, "yampt.tests.exe")
vcpkg_debug_bin = os.path.join(repo_root, "vcpkg_installed", "x64-windows", "x64-windows", "debug", "bin")
# A full Rebuild lets yampt.gui.tests' clean delete this from the shared OutDir.
dlls_lost_on_rebuild = ("Qt6Testd.dll",)
build_tool_processes = ("MSBuild.exe", "cl.exe", "link.exe")


def running_processes():
    output = subprocess.run(["tasklist", "/fo", "csv", "/nh"], capture_output=True, text=True).stdout
    return {line.split('","')[0].strip('"').lower() for line in output.splitlines() if line}


def wait_for_idle_build_tools(poll_seconds=10):
    while any(name.lower() in running_processes() for name in build_tool_processes):
        time.sleep(poll_seconds)


def kill_stray_tests():
    subprocess.run(["taskkill", "/f", "/im", "yampt.tests.exe"], capture_output=True)


def rebuild(log_path):
    with open(log_path, "w", encoding="utf-8", errors="replace") as log:
        completed = subprocess.run(
            [msbuild_path, os.path.join(repo_root, "yampt.sln"), "/t:Rebuild",
             "/p:Configuration=Debug", "/p:Platform=x64", "/v:minimal", "/nologo"],
            stdout=log, stderr=subprocess.STDOUT)
    return completed.returncode


def restore_lost_dlls():
    restored = []
    for name in dlls_lost_on_rebuild:
        target = os.path.join(debug_dir, name)
        source = os.path.join(vcpkg_debug_bin, name)
        if not os.path.exists(target) and os.path.exists(source):
            shutil.copy2(source, target)
            restored.append(name)
    return restored


def run_tests(xml_path, stall_seconds):
    if os.path.exists(xml_path):
        os.remove(xml_path)
    process = subprocess.Popen([test_exe, "-r", "xml", "-o", xml_path], cwd=debug_dir,
                               stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    last_size = -1
    last_change = time.time()
    while process.poll() is None:
        time.sleep(5)
        size = os.path.getsize(xml_path) if os.path.exists(xml_path) else 0
        if size != last_size:
            last_size = size
            last_change = time.time()
        elif time.time() - last_change > stall_seconds:
            process.kill()
            return None, True
    return process.returncode, False


def summarize_xml(xml_path):
    if not os.path.exists(xml_path):
        return {}
    with open(xml_path, "r", encoding="utf-8", errors="replace") as handle:
        text = handle.read()
    summary = {"cases_started": len(re.findall(r"<TestCase name=", text)),
               "complete": "</Catch2TestRun>" in text}
    totals = re.search(r'<OverallResultsCases successes="(\d+)" failures="(\d+)"', text)
    if totals:
        summary["cases_ok"] = int(totals.group(1))
        summary["cases_failed"] = int(totals.group(2))
    started = re.findall(r'<TestCase name="([^"]+)"[^>]*filename="([^"]+)"[^>]*line="(\d+)"', text)
    if started and not summary["complete"]:
        name, filename, line = started[-1]
        summary["stalled_in"] = f"{name} ({filename}:{line})"
    return summary


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--skip-build", action="store_true")
    parser.add_argument("--out-dir", default=tempfile.gettempdir())
    parser.add_argument("--stall-seconds", type=int, default=120)
    arguments = parser.parse_args()

    os.makedirs(arguments.out_dir, exist_ok=True)
    build_log = os.path.join(arguments.out_dir, "debug_rebuild.txt")
    xml_path = os.path.join(arguments.out_dir, "debug_tests.xml")
    done_path = os.path.join(arguments.out_dir, "debug_done.json")
    if os.path.exists(done_path):
        os.remove(done_path)

    result = {"build_exit": "skipped"}
    if not arguments.skip_build:
        wait_for_idle_build_tools()
        kill_stray_tests()
        result["build_exit"] = rebuild(build_log)

    if result["build_exit"] in (0, "skipped"):
        result["restored_dlls"] = restore_lost_dlls()
        kill_stray_tests()
        result["test_exit"], result["stalled"] = run_tests(xml_path, arguments.stall_seconds)
        result.update(summarize_xml(xml_path))

    with open(done_path, "w", encoding="utf-8") as handle:
        json.dump(result, handle, indent=2)
    return 0


if __name__ == "__main__":
    sys.exit(main())
