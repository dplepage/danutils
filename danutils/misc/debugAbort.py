import traceback, sys

_label = """
==== Debug Abort ====
In %(file)s, function %(fun)s, line %(line)i.
Message: %(msg)s
==== Debug Abort ====
""".strip()

_fields = ["file","line","fun","txt"]

def debugAbort(msg="No Message",code=-1):
	frameinfo = traceback.extract_stack(limit=2)[0]
	d = dict(zip(_fields,frameinfo))
	d['msg'] = msg
	print _label%d
	sys.exit(code)