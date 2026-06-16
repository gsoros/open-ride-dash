# version.py
import os

version_file = "version"
version = "0.0.0"
if os.path.exists(version_file):
    version = str(open(version_file).read().strip())


Import("env")
env.Append(CPPDEFINES=[("ORD_VERSION", f'\\"{version}\\"')])
