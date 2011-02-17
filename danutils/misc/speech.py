import subprocess
import atexit

def say(txt, voice=None):
    cmd = ['say']
    if voice:
        cmd.extend(['-v',voice])
    cmd.append(txt)
    p = subprocess.Popen(cmd, stderr=subprocess.PIPE)
    code = p.wait()
    if code != 0:
        raise OSError("'say' command failed: %s"%p.stderr.read())

def saywhendone(s):
	def _tmp():
		say(s)
	atexit.register(_tmp)
