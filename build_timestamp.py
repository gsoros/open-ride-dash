from datetime import datetime

Import("env")
ts = datetime.now().strftime("%Y-%m-%d_%H:%M:%S")
env.Append(CPPDEFINES=[("BUILD_TIMESTAMP", f'\\"{ts}\\"')])
