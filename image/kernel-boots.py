import argparse
import asyncio
import subprocess
import time
import os
from qemu.qmp import QMPClient


class KernelBoots:
  def __init__(self, qemu_cmd, timeout=30):
    self.qemu_process = None
    self.ffmpeg_process = None
    self.qmp = QMPClient()
    self.serial_log = os.getenv('ARTIFACTS_DIR') + '/serial.log'
    # console_handler = logging.StreamHandler()
    # console_handler.setLevel(logging.DEBUG)
    # formatter = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s')
    # console_handler.setFormatter(formatter)
    # self.qmp.logger.addHandler(console_handler)
    # self.qmp.logger.setLevel(logging.DEBUG)
    self.qemu_cmd = qemu_cmd
    self.qemu_cmd.extend(
      ['-vnc', ':1', '-no-reboot', '-qmp', 'unix:qmp-sock,server', '-S', '-serial', 'file:' + self.serial_log])
    self.timeout = timeout

  async def start_qemu(self):
    subprocess.run(
      ["cmake", "--build", os.getenv('CMAKE_BINARY_DIR'), "--target", "image", "--target", "image", "--target", "ovmf"])

    with open(self.serial_log, 'w') as serial_log:
      serial_log.truncate(0)

    os.remove('qmp-sock') if os.path.exists('qmp-sock') else None
    self.qemu_process = subprocess.Popen(self.qemu_cmd)
    while not os.path.exists('qmp-sock'):
      await asyncio.sleep(0.1)
    await self.qmp.connect('qmp-sock')
    print("Connected to QMP")
    asyncio.create_task(self.print_events())
    print("Continuing")
    await self.qmp.execute('cont')

  async def print_events(self):
    try:
      async for event in self.qmp.events:
        print(f"Event arrived: {event['event']}")
    except asyncio.CancelledError:
      return

  async def wait_for_output(self):
    serial_output = ""
    start_time = time.time()
    with open(self.serial_log, 'r') as serial_log:
      while time.time() - start_time < self.timeout:
        line = serial_log.readline()
        if line:
          print(line, end="")
          serial_output += line
          if "start complete" in serial_output:
            print("Kernel startup complete")
            break
        else:
          await asyncio.sleep(0.1)
      else:
        raise TimeoutError("Timeout reached before the kernel reached the specific point.")

  def __enter__(self):
    return self

  async def shutdown(self):
    print("shutting down")
    await self.qmp.execute('screendump', {'filename': os.getenv('ARTIFACTS_DIR') + '/screenshot.png', 'format': 'png'})
    try:
      await self.qmp.execute('quit')
    except Exception as e:
      print(f"Error executing quit command: {e}")

  def __exit__(self, exc_type, exc_val, exc_tb):
    try:
      self.qemu_process.wait(2)
      print("QEMU exited")
    except subprocess.TimeoutExpired:
      self.qemu_process.terminate()
      try:
        self.qemu_process.wait(10)
        print("QEMU terminated")
      except subprocess.TimeoutExpired:
        print("Failed to exit, killing the process")
        self.qemu_process.kill()


async def main(qemu_cmd):
  with KernelBoots(qemu_cmd) as kernel_boots:
    await kernel_boots.start_qemu()
    await kernel_boots.wait_for_output()
    print("Kernel boots")
    await kernel_boots.shutdown()


if __name__ == '__main__':
  parser = argparse.ArgumentParser(description='Start QEMU with specified arguments.')
  parser.add_argument('qemu_cmd', nargs=argparse.REMAINDER, help='QEMU command and arguments')
  args = parser.parse_args()

  if not args.qemu_cmd:
    parser.error("No QEMU command provided")

  asyncio.run(main(args.qemu_cmd))
