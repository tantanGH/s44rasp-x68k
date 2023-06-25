import os
import argparse
import time
import datetime
import signal
import serial
import subprocess

# API version
API_VERSION = "0.1"

# response codes
RESPONSE_OK                     = 200
RESPONSE_BAD_REQUEST            = 400
RESPONSE_NOT_FOUND              = 404
RESPONSE_INTERNAL_SERVER_ERROR  = 500
RESPONSE_SERVICE_UNAVAILABLE    = 503

# abort flag
g_abort_service = False

# sigint handler
def sigint_handler(signum, frame):
  print("CTRL-C is pressed. Stopping the service.")
  global g_abort_service
  g_abort_service = True

# respond
def respond(port, code, body=""):

  code_str = '<|{:04d}'.format(code)

  body_bytes = body.encode('cp932', errors="backslashreplace")
  body_len = len(body_bytes)
  body_len_str = '{:08x}'.format(body_len)

  res = bytearray()
  res.extend(code_str.encode('ascii'))
  res.extend(body_len_str.encode('ascii'))
  res.extend(body_bytes)
  port.write(res)
  port.flush()

# service loop
def run_service(serial_device, serial_baudrate, alsa_device, pcm_path):

  # set signal handler
  signal.signal(signal.SIGINT, sigint_handler)

  # open serial port
  with serial.Serial( serial_device, serial_baudrate,
                      bytesize = serial.EIGHTBITS,
                      parity = serial.PARITY_NONE,
                      stopbits = serial.STOPBITS_ONE,
                      timeout = 120,
                      xonxoff = False,
                      rtscts = False,
                      dsrdtr = False ) as port:

    print(f"Started. (serial_device={serial_device}, serial_baudrate={serial_baudrate}, alsa_device={alsa_device}, pcm_path={pcm_path})")

    global g_abort_service
    g_abort_service = False

    s44rasp_proc = None

    while g_abort_service is False:

      # find request header prefix '>', '|'
      prefix = 0
      while g_abort_service is False:
        if port.in_waiting < 1:
          time.sleep(0.05)
          continue
        c = port.read()
        if prefix == 0 and c[:1] == b'>':
          prefix = 1
        elif prefix == 1 and c[:1] == b'|':
          prefix = 2
          break
        else:
          prefix = 0

      # aborted?
      if prefix != 2:
        return

      # read request body size (8 byte hex string, 0 padding)
      request_body_size_bytes = port.read(8)
      request_body_size = int(request_body_size_bytes.decode('ascii'), 16)
      print(f"got request {request_body_size} bytes.")

      # request body
      request_body_bytes = port.read(request_body_size)
      request_body_str = request_body_bytes.decode('ascii')
      print(f"request: [{request_body_str}]")

      # request handler - version
      if request_body_str.startswith("/version"):
        print(f"response: [{API_VERSION}]")
        respond(port, RESPONSE_OK, API_VERSION)

      # request handler - 16bit PCM existence check with s44rasp
      elif request_body_str.startswith("/pcmhead?path="):

        request_path = request_body_str[14:]
        if ".." in request_path:
          respond(port, RESPONSE_BAD_REQUEST, "")
        else:
          pcm_file_name = pcm_path + "/" + request_path
          if os.path.isfile(pcm_file_name):
            pcm_file_size = os.path.getsize(pcm_file_name)
            respond(port, RESPONSE_OK, f"{pcm_file_size}")
          else:
            respond(port, RESPONSE_NOT_FOUND, "file not found.")

      # request handler - 16bit PCM play with s44rasp
      elif request_body_str.startswith("/pcmplay?path="):

        request_path = request_body_str[14:]
        if ".." in request_path:
          respond(port, RESPONSE_BAD_REQUEST, "")
        else:
          pcm_file_name = pcm_path + "/" + request_path
          if os.path.isfile(pcm_file_name):
            pcm_file_size = os.path.getsize(pcm_file_name)
            respond(port, RESPONSE_OK, f"{pcm_file_size}")
            if s44rasp_proc is not None:
              while s44rasp_proc.poll() is None:
                s44rasp_proc.kill()
            s44rasp_proc = subprocess.Popen(["s44rasp", "-d", alsa_device, "-o", pcm_file_name], shell=False)
          else:
            respond(port, RESPONSE_NOT_FOUND, "file not found.")

      # request handler - 16bit PCM stop
      elif request_body_str.startswith("/pcmstop"):
        if s44rasp_proc is not None:
          while s44rasp_proc.poll() is None:
            s44rasp_proc.kill()
          s44rasp_proc = None
        respond(port, RESPONSE_OK, f"stopped.")
      else:
        print(f"unknown request [{request_body_str}]")
        respond(port, RESPONSE_BAD_REQUEST)

    print("Stopped.")

# main
def main():

    parser = argparse.ArgumentParser()

    parser.add_argument("pcmpath", help="pcm data path")
    parser.add_argument("-a", "--alsa", help="alsa device name", default="hw:3,0")
    parser.add_argument("-d", "--port", help="serial device name", default='/dev/serial0')
    parser.add_argument("-s", "--baudrate", help="serial baud rate", type=int, default=38400)

    args = parser.parse_args()

    run_service(args.port, args.baudrate, args.alsa, args.pcmpath)


if __name__ == "__main__":
    main()
