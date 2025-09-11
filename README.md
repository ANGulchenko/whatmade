## What is whatmade?

Whatmade is a Linux daemon that monitors user-specified directories and records 
which process created each file. This makes it easy to later identify the origin 
of files with suspicious or unexpected names.    
The main idea was to monitor and identify files in the dot directories in the /home.

## Building?

```bash
  mkdir build
  cd build
  cmake ..
  make
```

## Autostart through systemd?

Copy the ./systemd/whatmade.service file to /etc/systemd/system/ and enable the service with
these commands:
```bash
sudo systemctl daemon-reload
sudo systemctl enable whatmade.service
sudo systemctl start whatmade.service
```

## How to use?

Copy the whatmade binary to somewhere covered with PATH environment path and run the daemon
manually or through the provided systemd service script.

It will create the config file /etc/whatmade/config.cfg with 2 arrays for 
monitoring and ignoring directories. These directories include subdirectories 
(including mount points if you have such) so no need to add them separately.

Restart daemon to let it read the config. 

That's all for configuration.

### CLI

    -h  --help                     Print parameters list
    -w  --what <filename>          What created this file?(Human-redable format)
    -r  --raw  <filename>          What created this file?(Raw format)
    -d  --dir  <dirname>           Directory summary(including subdirectories)
    Shows the table <process name> <number of files made by this process> <Total filesize>
    -c  --clear <file/dirname>     Clears process saved data from file or all files in the dir
    This only removes data from extended attributes that was previously written by this daemon
    -n  --non_daemonize            Run without daemonizing(useful for running through systemd)
    -s  --stop                     Stop and quit (useful if run as daemon)

## Nuances?

Whatmade uses the Linux fanotify API, which can be a bit… unusual and doesn’t 
always behave as you might expect. At present, the daemon doesn’t strictly 
detect file **creation** events. Instead, it logs processes that **access** 
files (applying some trivial filters like analyzing the age of file), which can 
sometimes lead to false positives.

While this does not make the daemon useless, it does mean that the information 
it provides should be interpreted with care — critical thinking is still 
advised when reviewing the results.

This daemon stores process data in the Extended File Attributes.

## Convenience?

### Caja
I offer you an extension for the Caja FM (from MATE DE, fork of Gnome 2) where you
can just right-click on file and ask "What made this?" from a menu.
Put the file in ~/.local/share/caja-python/extensions and restart the Caja ("caja -q")

![screenshot](./FM_Extensions/MATE-CAJA/whatmade_win.png)

### Midnight Commander
You can add a new entity to mc's User Menu (F2): ~/.config/mc/menu
```
w What process made this file?
    whatmade -w "%p"
```
This code will add a new line in that menu with "w" shortkey.

## Dependencies?

 - libconfig++
