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

