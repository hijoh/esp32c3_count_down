set remotetimeout 100
@REM gdb_port 0.0.0.0:3333
target extended-remote :3333
set remote hardware-watchpoint-limit 2
mon reset halt
flushregs
thb app_main
c