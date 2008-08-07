Thinks not fixed to a release anymore:


critical:
http://bugs.debian.org/cgi-bin/bugreport.cgi?bug=470882
http://bugs.gentoo.org/show_bug.cgi?id=219577



normal:

- finish gpm2/doc/DESIGN
- fix checksum path (is .. currently)

- fix dependencies...: *.d, *.P!
- check for rmev (1.19.6 or 1.17.8)
- try to implement n mice by usage of add_mouse
- add help for calib file in English
- add document, that describes on howto add a new driver

- Fix open_console: serial check is missing
- write / find indent script to fixup source files

- check for senseful use of global variables (gunze_calib, etouch
   -> also in the library code

gpm2:
- protocols:
   GPM2_PROTOCOLS via cconf
      wenn leer, dann ls mice/*
- version
   via cconf!



- double check that the os speficih code is in gpm2/os/<name>/
   prefix os specific drivers with OS-name?
      like linux event and joystick
- Fix Gpm_GetServerVersion and Gpm_GetLibVersion
   -> Gpm_GetServerVersion: connect to server and ask
   -> Gpm_GetLibVersion: use real library version
