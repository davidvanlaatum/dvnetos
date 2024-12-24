import sys
import subprocess
import json
import os
import re
import select
from typing import List, Optional


class WarningRec:
  def __init__(self, file, line, column, level, message):
    self.file = file
    self.line = line
    self.column = column
    self.level = level
    self.message = message
    self.count = 1
    self.extra = None
    self.notes: List[WarningRec] = []

  def append_extra(self, extra):
    if not extra:
      return
    if not self.extra:
      self.extra = ""
    self.extra += extra

  def add_note(self, note: 'WarningRec'):
    self.notes.append(note)

  def is_note(self):
    return self.level == 'note'

  def __eq__(self, other):
    if isinstance(other, WarningRec):
      return (self.file == other.file and
              self.line == other.line and
              self.column == other.column and
              self.level == other.level and
              self.message == other.message)
    return False

  def add_to_count(self, num):
    self.count += num

  def to_dict(self):
    return {
      'file': self.file,
      'line': self.line,
      'column': self.column,
      'level': self.level,
      'message': self.message,
      'count': self.count,
      'extra': self.extra,
      'notes': self.notes
    }

  def to_string(self):
    s = self.file + ':' + str(self.line) + ':' + str(self.column) + ':' + self.level + ':' + self.message
    if self.count > 1:
      s += ' x' + str(self.count)
    s += '\n'
    if self.extra:
      s += self.extra
    if len(self.notes) > 0:
      for note in self.notes:
        s += note.to_string()
    return s

  @classmethod
  def from_dict(cls, data):
    instance = cls(data['file'], data['line'], data['column'], data['level'], data['message'])
    instance.count = data['count']
    instance.extra = data.get('extra')
    instance.notes = [cls.from_dict(note) for note in data.get('notes', [])]
    return instance


class WarningCollector:
  def __init__(self):
    self.warnings: List[WarningRec] = []
    self.inWarning = False
    self.currentWarning: Optional[WarningRec] = None
    self.lastWarning: Optional[WarningRec] = None

  def collect(self, line):
    line = re.sub(r'\x1B\[[0-9;]*[A-Za-z]', '', line)
    if self.inWarning:
      if re.match(r'^\s+', line):
        self.currentWarning.append_extra(line)
      else:
        if not self.currentWarning.is_note():
          self.lastWarning = self.currentWarning
        self.inWarning = False
        self.currentWarning = None
    match = re.match(r'^(.*):(\d+):(\d+):\s+([a-zA-Z]+):\s+(.*)$', line)
    if match:
      self.currentWarning = WarningRec(match.group(1), int(match.group(2)), int(match.group(3)), match.group(4),
                                       match.group(5))
      self.inWarning = True
      if self.currentWarning.is_note() and self.lastWarning:
        self.lastWarning.add_note(self.currentWarning)
      else:
        self.warnings.append(self.currentWarning)

  def dump_to_json_file(self, file):
    if len(self.warnings) > 0:
      with open(file, 'w') as f:
        json.dump({'warnings': self.warnings}, f, indent=2,
                  default=lambda o: o.to_dict() if hasattr(o, 'to_dict') else o)

  def dump_to_markdown_file(self, file):
    if len(self.warnings) > 0:
      with open(file, 'a') as f:
        f.write('<details><summary>' + str(len(self.warnings)) + ' Warnings</summary>\n\n```\n')
        for w in self.warnings:
          f.write(w.to_string() + "\n")
        f.write('````\n</details>')

  def merge(self, other: 'WarningCollector'):
    for w in other.warnings:
      found = False
      for w2 in self.warnings:
        if w == w2:
          w2.add_to_count(w.count)
          found = True
          break
      if not found:
        self.warnings.append(w)

  @classmethod
  def load(cls, file):
    with open(file, 'r') as f:
      data = json.load(f)
      collector = cls()
      collector.warnings = [WarningRec.from_dict(w) for w in data['warnings']]
      return collector

  def load_and_merge(self, file):
    self.merge(WarningCollector.load(file))


class ProcessRunner:
  def __init__(self, cmd, mode):
    self.cmd = cmd
    self.mode = mode
    self.collector = WarningCollector()

  def run(self):
    process = subprocess.Popen(self.cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    stderr_collector = WarningCollector()
    stdout_collector = WarningCollector()
    while True:
      reads = [process.stdout.fileno(), process.stderr.fileno()]
      ret = select.select(reads, [], [])

      stdout_line = None
      stderr_line = None

      for fd in ret[0]:
        if fd == process.stdout.fileno():
          stdout_line = process.stdout.readline()
          if stdout_line:
            print(stdout_line, end="")
            stdout_collector.collect(stdout_line)
        elif fd == process.stderr.fileno():
          stderr_line = process.stderr.readline()
          if stderr_line:
            print(stderr_line, end="")
            stderr_collector.collect(stderr_line)

      if process.poll() is not None and not stdout_line and not stderr_line:
        break

    process.stdout.close()
    process.stderr.close()
    process.wait()

    self.collector.merge(stdout_collector)
    self.collector.merge(stderr_collector)
    self.collector.dump_to_json_file(self.get_output_file() + '.' + self.mode + '.warnings.json')

    return process.returncode

  def get_collector(self):
    return self.collector

  def get_output_file(self):
    for i, arg in enumerate(self.cmd):
      if arg == '-o' and i + 1 < len(self.cmd):
        return self.cmd[i + 1]
    return None


class ReportGenerator:
  def __init__(self):
    self.collector = WarningCollector()

  def load_from_list(self, file):
    with open(file, 'r') as f:
      for line in f:
        try:
          self.collector.load_and_merge(line.strip())
        except FileNotFoundError:
          pass

  def write_report(self):
    if os.getenv('GITHUB_STEP_SUMMARY'):
      self.collector.dump_to_markdown_file(os.getenv('GITHUB_STEP_SUMMARY'))


def main():
  if len(sys.argv) > 1 and sys.argv[1] == 'report':
    report = ReportGenerator()
    print('reading', sys.argv[2])
    report.load_from_list(sys.argv[2])
    report.write_report()
  elif len(sys.argv) > 1:
    runner = ProcessRunner(sys.argv[2:], sys.argv[1])
    sys.exit(runner.run())
  else:
    print('Usage: warningcollector.py [report] [filelistfile]')
    print(' or ')
    print('Usage: warningcollector.py [link|compile|clang-tidy] [cmd...]')
    sys.exit(1)


if __name__ == '__main__':
  main()
