# cyberspace-kiosk

This is a C-based server that allows you to download zipped files/folders across a local network, served from a ww/ directory

### _NOTE! IT HAS A SMALL FORMAT!_
```
kiosk (executable)
kiosk.c 
www/
    src/
        (whatever)
    ShowFile
```
ShowfFiles generally look like this:
```
name:
description:
author:
```
and they're the backbone behind showing stuff actually existing

Intentionally 1990's themed, give or take. 

---

Compilation via  `tcc -o kiosk kiosk.c`
