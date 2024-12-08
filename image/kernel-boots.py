import argparse
import asyncio
import logging
import subprocess
import time
import os
from qemu.qmp import QMPClient
import base64


class KernelBoots:
  def __init__(self, qemu_cmd, timeout=30):
    self.logger = logging.getLogger('KernelBoots')
    # self.logger.setLevel(logging.DEBUG)
    self.qemu_process = None
    self.ffmpeg_process = None
    self.qmp = QMPClient()
    self.serial_log = os.getenv('ARTIFACTS_DIR') + '/serial.log'
    self.summary_path = os.getenv('GITHUB_STEP_SUMMARY')
    self.serial_output = ""
    # console_handler = logging.StreamHandler()
    # console_handler.setLevel(logging.DEBUG)
    # formatter = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s')
    # console_handler.setFormatter(formatter)
    # self.qmp.logger.addHandler(console_handler)
    # self.qmp.logger.setLevel(logging.DEBUG)
    self.qemu_cmd = qemu_cmd
    self.qemu_cmd.extend(
      ['-display', 'none', '-no-reboot', '-qmp', 'unix:qmp-sock,server', '-S', '-serial', 'file:' + self.serial_log])
    self.timeout = timeout
    self.logger.info(f"QEMU command: {self.qemu_cmd} timeout: {self.timeout}")

  async def start_qemu(self):
    subprocess.run(
      ["cmake", "--build", os.getenv('CMAKE_BINARY_DIR'), "--target", "image", "--target", "image", "--target", "ovmf"])
    self.logger.info("Image Built")

    with open(self.serial_log, 'w') as serial_log:
      serial_log.truncate(0)

    os.remove('qmp-sock') if os.path.exists('qmp-sock') else None
    self.logger.info("Starting QEMU")
    self.qemu_process = subprocess.Popen(self.qemu_cmd)
    while not os.path.exists('qmp-sock'):
      await asyncio.sleep(0.1)
    self.logger.info("QEMU started")
    await self.qmp.connect('qmp-sock')
    self.logger.info("Connected to QMP")
    asyncio.create_task(self.print_events())
    self.logger.info("Continuing")
    await self.qmp.execute('cont')

  async def print_events(self):
    try:
      async for event in self.qmp.events:
        self.logger.info(f"Event arrived: {event['event']}")
    except asyncio.CancelledError:
      return

  async def wait_for_output(self):
    self.serial_output = ""
    start_time = time.time()
    with open(self.serial_log, 'r') as serial_log:
      while time.time() - start_time < self.timeout:
        line = serial_log.readline()
        if line:
          print(line, end="", flush=True)
          self.serial_output += line
          if "start complete" in self.serial_output:
            self.logger.info("Kernel startup complete")
            break
        else:
          await asyncio.sleep(0.1)
      else:
        raise TimeoutError("Timeout reached before the kernel reached the specific point.")

  def __enter__(self):
    return self

  async def shutdown(self):
    self.logger.info("shutting down")
    try:
      screenshot_path = os.getenv('ARTIFACTS_DIR') + '/screenshot.png'
      await self.qmp.execute('screendump', {'filename': screenshot_path, 'format': 'png'})
      self.add_to_summary(screenshot_path)
    except Exception as e:
      self.logger.error(f"Error executing screendump command: {e}")
    try:
      await self.qmp.execute('quit')
    except Exception as e:
      self.logger.error(f"Error executing quit command: {e}")

  def add_to_summary(self, image_path):
    if not self.summary_path:
      self.logger.info("No summary path provided")
      return
    with open(image_path, 'rb') as image_file:
      image_data = base64.b64encode(image_file.read()).decode('utf-8')
    with open(self.summary_path, 'a') as summary_file:
      summary_file.write(f"<img src=\"data:image/png;base64,{image_data}\" />\n")
      summary_file.write(
        f"<details><summary>Kernel Output</summary><pre style=\"max-height: 250px; overflow: auto\">{self.serial_output}</pre></details>\n")
    self.logger.info("Wrote screen shot to summary")

  def __exit__(self, exc_type, exc_val, exc_tb):
    try:
      self.qemu_process.wait(2)
      self.logger.info("QEMU exited")
    except subprocess.TimeoutExpired:
      self.qemu_process.terminate()
      try:
        self.qemu_process.wait(10)
        self.logger.info("QEMU terminated")
      except subprocess.TimeoutExpired:
        self.logger.warning("Failed to exit, killing the process")
        self.qemu_process.kill()
        try:
          self.qemu_process.wait(10)
          self.logger.info("QEMU killed")
        except subprocess.TimeoutExpired:
          self.logger.error("Failed to kill the process")


async def main(qemu_cmd, timeout):
  with KernelBoots(qemu_cmd, timeout) as kernel_boots:
    try:
      await kernel_boots.start_qemu()
      await kernel_boots.wait_for_output()
      kernel_boots.logger.info("Kernel boots")
    finally:
      await kernel_boots.shutdown()


if __name__ == '__main__':
  parser = argparse.ArgumentParser(description='Start QEMU with specified arguments.')
  parser.add_argument('--timeout', type=int, help='Timeout in seconds', default=30)
  parser.add_argument('--debug', action='store_true', help='Enable debug logging')
  parser.add_argument('qemu_cmd', nargs=argparse.REMAINDER, help='QEMU command and arguments')
  args = parser.parse_args()

  if not args.qemu_cmd:
    parser.error("No QEMU command provided")

  logging.basicConfig(
    level=logging.INFO if not args.debug else logging.DEBUG,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s',
    handlers=[logging.StreamHandler()]
  )

  try:
    asyncio.run(main(args.qemu_cmd, args.timeout))
  except Exception as e:
    print(f"Error: {e}")
    exit(1)
