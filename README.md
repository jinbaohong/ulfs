# ulfs

## EXT2 interface
```bash
gcc -g show_ext2.c -o show_ext2
./show_ext2 mydisk /f1/k2/hello
```
### Example
```bash
$ ./show_ext2 mydisk /f1/k2/hello
15
-rw-r--r--
size = 19
i_ctime : Sun May 17 15:10:46 2020
user = jinbao
group = jinbao
$ ls -li mnt/f1/k2/hello 
15 -rw-r--r-- 1 jinbao jinbao 19  5月 16 17:52 mnt/f1/k2/hello
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
