#!/bin/sh

# system.wmmount.awk - Generation of system-wide config file for wmmount
# 05/09/98  Release 1.0 Beta1
# Copyright (C) 1998  Sam Hawker <shawkie@geocities.com>
# This software comes with ABSOLUTELY NO WARRANTY
# This software is free software, and you are welcome to redistribute it
# under certain conditions
# See the README file for a more complete notice.

# Usage:
# ./system.wmmount.awk >system.wmmount

# ------------------------------------------------------------------------

echo "\
# wmmount - The WindowMaker universal mount point
# 05/09/98  Release 1.0 Beta1
# Copyright (C) 1998  Sam Hawker <shawkie@geocities.com>
# This software comes with ABSOLUTELY NO WARRANTY
# This software is free software, and you are welcome to redistribute it
# under certain conditions
# See the README file for a more complete notice.

# System config file.
# Should be saved as /usr/X11R6/lib/X11/wmmount/system.wmmount
# Comments and blank lines mostly allowed.
# Do not indent lines unless it is explicitly requested.

# First, tell wmmount how to mount and unmount devices.
# %s will be replaced with the appropriate mountpoint.
# Since only the mountpoint will be passed to mount, the
# device must be listed in /etc/fstab.
# These entries may be ommitted (see wmmount.cc for defaults).

#mountcmd=mount %s &
#umountcmd=umount %s &

# Next, tell wmmount what to do when you double-click on the
# info box for a mounted device.
# Again %s will be replaced with the mountpoint.
# This entry may be ommitted (see wmmount.cc).

#opencmd=kfmclient exec %s &

# Next, choose some fonts and colors for the information.
# These entries may be ommitted (see wmmount.cc).

# backcolor  The background color of the info box.
# ntxtfont   The font used for the mount name text.
# ntxtcolor  The color of the mount name text.
# utxtfont   The font used for the usage display text.
# utxtcolor  The color of the usage display text.

#backcolor=#282828
#ntxtfont=-*-helvetica-bold-r-*-*-10-*-*-*-*-*-*-*
#ntxtcolor=gray90
#utxtfont=-*-helvetica-medium-r-*-*-10-*-*-*-*-*-*-*
#utxtcolor=gray90

# Now list all the icons you want to use.
# You do not need to list any file more than once.
# The first icon specified gets iconnumber 0, the
# next iconnumber 1, etc.

# At least one icon must be specified!
# References to non-existent files will cause crashes.

icon /usr/X11R6/lib/X11/wmmount/cdrom.xpm
icon /usr/X11R6/lib/X11/wmmount/floppy.xpm
icon /usr/X11R6/lib/X11/wmmount/zip.xpm
icon /usr/X11R6/lib/X11/wmmount/harddisk.xpm

# Now give details of each mountpoint.
# Each mountpoint requires 4 *consecutive* lines.

# eg.

#mountname cdrom
#   mountpoint /mnt/cdrom
#   iconnumber 0
#   usagedisplay Free

# mountname     The name to be displayed in the info box.
# mountpoint    The directory where the device is mounted.
# iconnumber    A reference to an icon specified above.
# usagedisplay  The type of usage information to be displayed
#               (can be Free, Used, %Capacity or None).

# Details of at least one mointpoint must be specified.
# References to invalid iconnumbers will cause crashes.
"

awk -- "
{INUM=3 ; UDISP=\"Free\"}
{if (\$0 ~ /^\#/) {next}}
{if (\$0 ~ /^$/) {next}}
{if (\$2 ~ /^\/proc$/) {next}}
{if (\$1 ~ /fd/ || \$2 ~ /floppy/) {INUM=1 ; UDISP=\"Free\"}}
{if (\$1 ~ /cdrom/ || \$2 ~ /cdrom/) {INUM=0 ; UDISP=\"None\"}}
{if (\$1 ~ /zip/ || \$2 ~ /zip/) {INUM=2 ; UDISP=\"Free\"}}
{if (\$2 ~ /^\//) {
{if (\$2 ~ /^\/\$/)
{print \"mountname $(uname)\"}
else
{print \"mountname \" toupper(substr(\$2,match(\$2, /\/[^\/]*\$/)+1,1)) substr(\$2,match(\$2, /\/[^\/]*\$/)+2)}
}
{print \"   mountpoint \" \$2}
{print \"   iconnumber \" INUM}
{print \"   usagedisplay \" UDISP \"\\n\"}
}}
" /etc/fstab
