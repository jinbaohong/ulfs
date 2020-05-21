# ulfs

## EXT2 fs Implement
```bash
dd if=/dev/zero of=mydisk bs=1024 count=1440
mke2fs mydisk 1440
gcc FS1.1.c -o FS1.1
./FS1.1 
```
### Example
```bash
$ ./FS1.1
bmap=8 imap=9 iblock=10
mount : mydisk mounted on / 
P0 running: input command : pwd
Running pwd
/
********MINODE**[dev ino refCount]*******
[3 2 4]->[0 0 0]->[0 0 0]->[0 0 0]->[0 0 0]->[0 0 0]->[0 0 0]->[0 0 0]->[0 0 0]->[0 0 0]->
running->cwd->ino = 2
*****************************************
P0 running: input command : cd f1
********MINODE**[dev ino refCount]*******
[3 2 3]->[3 12 1]->[0 0 0]->[0 0 0]->[0 0 0]->[0 0 0]->[0 0 0]->[0 0 0]->[0 0 0]->[0 0 0]->
running->cwd->ino = 12
*****************************************
P0 running: input command : pwd
Running pwd
/f1/
********MINODE**[dev ino refCount]*******
[3 2 4]->[3 12 1]->[0 0 0]->[0 0 0]->[0 0 0]->[0 0 0]->[0 0 0]->[0 0 0]->[0 0 0]->[0 0 0]->
running->cwd->ino = 12
*****************************************

```

## EXT2 traverse
```bash
gcc -g show_ext2.c -o show_ext2
./show_ext2 mydisk /f1/k2/hello
```
### Example
```bash
$ ./show_ext2 mydisk /f1/k2/
13 drwxr-xr-x 3 jinbao jinbao 1024  May 17 20:00 .
12 drwxr-xr-x 3 jinbao jinbao 1024  May 16 17:52 ..
17 -rw-rw-r-- 1 jinbao jinbao    0  May 17 19:59 foo
15 -rw-r--r-- 1 jinbao jinbao   19  May 16 17:52 hello
19 -rw-rw-r-- 1 jinbao jinbao    0  May 17 19:59 bar
20 drwxrwxr-x 2 jinbao jinbao 1024  May 17 20:00 f4
$ ls -lia mnt/f1/k2
13 drwxr-xr-x 3 jinbao jinbao 1024  5月 17 20:00 .
12 drwxr-xr-x 3 jinbao jinbao 1024  5月 16 17:52 ..
19 -rw-rw-r-- 1 jinbao jinbao    0  5月 17 19:59 bar
20 drwxrwxr-x 2 jinbao jinbao 1024  5月 17 20:00 f4
17 -rw-rw-r-- 1 jinbao jinbao    0  5月 17 19:59 foo
15 -rw-r--r-- 1 jinbao jinbao   19  5月 16 17:52 hello

```
## FileSystem operation simulator
```bash
gcc ulfs.c -o ulfs
./ulfs
```
### Example
```bash
ulfs:/$ mkdir a
ulfs:/$ cd a
ulfs:/a/$ mkdir b
ulfs:/a/$ cd b
ulfs:/a/b/$ mkdir c
ulfs:/a/b/$ mkdir c1
ulfs:/a/b/$ mkdir c2
ulfs:/a/b/$ cd /
ulfs:/$ ls a/b
c  c1  c2  
ulfs:/$ save
```
