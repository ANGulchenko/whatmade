## 0.2.0
  - Backward compatibility is broken. Be careful.
  - Now data uses \0 as a separator between process name and parameters instead 
  of previously used space. It is important and will help to avoid any problems with
  spaces in paths and process names.
  - CLI is slightly changed: -w is for human-readable output, -r for raw, script convenient, format.
  - New -c "--clear" parameter for removing process data from a single file or all files in a directory (including subdirectories)
  - New -d "--dir" parameter for printing out the short summary for the dir (process name, number of files, total size of those files)
  - Some refactoring: mostly translating C to C++.

## 0.1.1
  - Mounting points from the subdirs of watched directories are now added automatically.
  So, if you have a mount point /home but want to watch the /home/user dir with all its 
  subdirs including /home/user/mounted_HDD_with_movies, you shouldn't add anything to the config.
  /home/user will suffice.

## 0.1.0
  - Data is now stored in file extended attributes
  - Got rid of DB
  - Config is in /etc/whatmade/config.cfg


## 0.0.1 (untested)
  - initial version
  - daemonizing
  - autocleaning DB (already deleted files)

