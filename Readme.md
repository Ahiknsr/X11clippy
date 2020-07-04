# About

worried about clipboard snooping? just run clippy

clippy will run in the background listening for clipboard events and will send a notification for every paste request

# Limitations

[INCR mechanisim](https://www.x.org/releases/X11R7.6/doc/xorg-docs/specs/ICCCM/icccm.html#incr_properties) is not implemented so the size of clipboard is limited. In my system i was able to copy upto 260000 bytes. This will not cause any issue when you are copying huge files/folders in filemanagers as filemanagers only store the paths of file/folders in clipboard.

# credits
https://www.uninformativ.de/blog/postings/2017-04-02/0/POSTING-en.html