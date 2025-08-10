## What is whomade?

Whomade is a Linux daemon that monitors user-specified directories and records 
which process created each file. This makes it easy to later identify the origin 
of files with suspicious or unexpected names.

## Nuances?

Whomade uses the Linux fanotify API, which can be a bit… unusual and doesn’t 
always behave as you might expect. At present, the daemon doesn’t strictly 
detect file **creation** events. Instead, it logs processes that **access** 
files (applying some trivial filters like analyzing the age of file), which can 
sometimes lead to false positives.

While this does not make the daemon useless, it does mean that the information 
it provides should be interpreted with care — critical thinking is still 
advised when reviewing the results.

## Dependancies?

Only libsqlite3.
