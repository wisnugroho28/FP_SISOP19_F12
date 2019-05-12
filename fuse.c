#define FUSE_USE_VERSION 28
#define HAVE_SETXATTR

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef linux
/* For pread()/pwrite() */
#define _XOPEN_SOURCE 500
#endif

#include <pthread.h>
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <sys/time.h>
#ifdef HAVE_SETXATTR
#include <sys/xattr.h>
#endif

#define DIRPATH "/home/mungkin"

char songList[1000][1000];
int myIndex=0;

/*fungsi untuk check ekstensi file*/
bool extensionMatch(const char *name, const char *ext);

/*fungsi untuk copy file make exec fork*/
int copyFile(const char *dest, const char *source);

/*fungsi untuk mengcopy file mp3 dari seluruh folder didalam folder root*/
void myMove(char* path);

//TEMPLATE
void xmp_destroy(void *private_data){
    if(chdir(DIRPATH)<0){
        exit(0);
    }
    int j;
    for(j=0;j<myIndex;j++){
        remove(songList[j]);
    }
}

static int xmp_getattr(const char *path, struct stat *stbuf)
{
	int res;
	char fullPath[1000];
	sprintf(fullPath,"%s%s",DIRPATH,path);

	res = lstat(fullPath, stbuf);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_access(const char *path, int mask)
{
	int res;
	char fullPath[1000];
	sprintf(fullPath,"%s%s",DIRPATH,path);

	res = access(fullPath, mask);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_readlink(const char *path, char *buf, size_t size)
{
	int res;
	char fullPath[1000];
	sprintf(fullPath,"%s%s",DIRPATH,path);

	res = readlink(fullPath, buf, size - 1);
	if (res == -1)
		return -errno;

	buf[res] = '\0';
	return 0;
}


static int xmp_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
		       off_t offset, struct fuse_file_info *fi)
{
    char fullPath[1000];
	if(strcmp(path,"/") == 0)
	{
		path=DIRPATH;
		sprintf(fullPath,"%s",path);
	}
	else sprintf(fullPath, "%s%s",DIRPATH,path);
	DIR *dp;
	struct dirent *de;

	(void) offset;
	(void) fi;

	dp = opendir(path);
	if (dp == NULL)
		return -errno;

	while ((de = readdir(dp)) != NULL) {
		struct stat st;
		memset(&st, 0, sizeof(st));
		st.st_ino = de->d_ino;
		st.st_mode = de->d_type << 12;
        
        if(S_ISDIR(st.st_mode) || !extensionMatch(de->d_name, ".mp3")){
            continue;
        }
		if (filler(buf, de->d_name, &st, 0))
			break;
	}

	closedir(dp);
	return 0;
}

static int xmp_mknod(const char *path, mode_t mode, dev_t rdev)
{
	int res;
	char fullPath[1000];
	sprintf(fullPath,"%s%s",DIRPATH,path);

	/* On Linux this could just be 'mknod(fullPath, mode, rdev)' but this
	   is more portable */
	if (S_ISREG(mode)) {
		res = open(fullPath, O_CREAT | O_EXCL | O_WRONLY, mode);
		if (res >= 0)
			res = close(res);
	} else if (S_ISFIFO(mode))
		res = mkfifo(fullPath, mode);
	else
		res = mknod(fullPath, mode, rdev);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_mkdir(const char *path, mode_t mode)
{
	int res;
	char fullPath[1000];
	sprintf(fullPath,"%s%s",DIRPATH,path);

	res = mkdir(fullPath, mode);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_unlink(const char *path)
{
	int res;
	char fullPath[1000];
	sprintf(fullPath,"%s%s",DIRPATH,path);

	res = unlink(fullPath);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_rmdir(const char *path)
{
	int res;
	char fullPath[1000];
	sprintf(fullPath,"%s%s",DIRPATH,path);

	res = rmdir(fullPath);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_symlink(const char *from, const char *to)
{
	int res;
	char fullPath[1000], tpath[1000];
	sprintf(fullPath,"%s%s",DIRPATH,from);
	sprintf(tpath,"%s%s",DIRPATH,to);

	res = symlink(fullPath, tpath);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_rename(const char *from, const char *to)
{
	int res;
	char fullPath[1000], tpath[1000];
	sprintf(fullPath,"%s%s",DIRPATH,from);
	sprintf(tpath,"%s%s",DIRPATH,to);

	res = rename(fullPath, tpath);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_link(const char *from, const char *to)
{
	int res;
	char fullPath[1000], tpath[1000];
	sprintf(fullPath,"%s%s",DIRPATH,from);
	sprintf(tpath,"%s%s",DIRPATH,to);

	res = link(fullPath, tpath);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_chmod(const char *path, mode_t mode)
{
	int res;
	char fullPath[1000];
	sprintf(fullPath,"%s%s",DIRPATH,path);

	res = chmod(fullPath, mode);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_chown(const char *path, uid_t uid, gid_t gid)
{
	int res;
	char fullPath[1000];
	sprintf(fullPath,"%s%s",DIRPATH,path);

	res = lchown(fullPath, uid, gid);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_truncate(const char *path, off_t size)
{
	int res;
	char fullPath[1000];
	sprintf(fullPath,"%s%s",DIRPATH,path);

	res = truncate(fullPath, size);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_utimens(const char *path, const struct timespec ts[2])
{
	int res;
	char fullPath[1000];
	sprintf(fullPath,"%s%s",DIRPATH,path);
	struct timeval tv[2];

	tv[0].tv_sec = ts[0].tv_sec;
	tv[0].tv_usec = ts[0].tv_nsec / 1000;
	tv[1].tv_sec = ts[1].tv_sec;
	tv[1].tv_usec = ts[1].tv_nsec / 1000;

	res = utimes(fullPath, tv);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_open(const char *path, struct fuse_file_info *fi)
{
	int res;
	char fullPath[1000];
	sprintf(fullPath,"%s%s",DIRPATH,path);

	res = open(fullPath, fi->flags);
	if (res == -1)
		return -errno;

	close(res);
	return 0;
}

static int xmp_read(const char *path, char *buf, size_t size, off_t offset,
		    struct fuse_file_info *fi)
{
	int fd;
	int res;
	char fullPath[1000];
	sprintf(fullPath,"%s%s",DIRPATH,path);

	(void) fi;
	fd = open(fullPath, O_RDONLY);
	if (fd == -1)
		return -errno;

	res = pread(fd, buf, size, offset);
	if (res == -1)
		res = -errno;

	close(fd);
	return res;
}

static int xmp_write(const char *path, const char *buf, size_t size,
		     off_t offset, struct fuse_file_info *fi)
{
	int fd;
	int res;
	char fullPath[1000];
	sprintf(fullPath,"%s%s",DIRPATH,path);

	(void) fi;
	fd = open(fullPath, O_WRONLY);
	if (fd == -1)
		return -errno;

	res = pwrite(fd, buf, size, offset);
	if (res == -1)
		res = -errno;

	close(fd);
	return res;
}

static int xmp_statfs(const char *path, struct statvfs *stbuf)
{
	int res;
	char fullPath[1000];
	sprintf(fullPath,"%s%s",DIRPATH,path);

	res = statvfs(fullPath, stbuf);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_create(const char* path, mode_t mode, struct fuse_file_info* fi) {

    (void) fi;

    int res;
	char fullPath[1000];
	sprintf(fullPath,"%s%s",DIRPATH,path);
    res = creat(fullPath, mode);
    if(res == -1)
	return -errno;

    close(res);

    return 0;
}


static int xmp_release(const char *path, struct fuse_file_info *fi)
{
	/* Just a stub.	 This method is optional and can safely be left
	   unimplemented */

	(void) path;
	(void) fi;
	return 0;
}

static int xmp_fsync(const char *path, int isdatasync,
		     struct fuse_file_info *fi)
{
	/* Just a stub.	 This method is optional and can safely be left
	   unimplemented */

	(void) path;
	(void) isdatasync;
	(void) fi;
	return 0;
}

#ifdef HAVE_SETXATTR
static int xmp_setxattr(const char *path, const char *name, const char *value,
			size_t size, int flags)
{
	int res = lsetxattr(path, name, value, size, flags);
	if (res == -1)
		return -errno;
	return 0;
}

static int xmp_getxattr(const char *path, const char *name, char *value,
			size_t size)
{
	int res = lgetxattr(path, name, value, size);
	if (res == -1)
		return -errno;
	return res;
}

static int xmp_listxattr(const char *path, char *list, size_t size)
{
	int res = llistxattr(path, list, size);
	if (res == -1)
		return -errno;
	return res;
}

static int xmp_removexattr(const char *path, const char *name)
{
	int res = lremovexattr(path, name);
	if (res == -1)
		return -errno;
	return 0;
}
#endif /* HAVE_SETXATTR */

static struct fuse_operations xmp_oper = {
    .destroy    = xmp_destroy,
	.getattr	= xmp_getattr,
	.access		= xmp_access,
	.readlink	= xmp_readlink,
	.readdir	= xmp_readdir,
	.mknod		= xmp_mknod,
	.mkdir		= xmp_mkdir,
	.symlink	= xmp_symlink,
	.unlink		= xmp_unlink,
	.rmdir		= xmp_rmdir,
	.rename		= xmp_rename,
	.link		= xmp_link,
	.chmod		= xmp_chmod,
	.chown		= xmp_chown,
	.truncate	= xmp_truncate,
	.utimens	= xmp_utimens,
	.open		= xmp_open,
	.read		= xmp_read,
	.write		= xmp_write,
	.statfs		= xmp_statfs,
	.create         = xmp_create,
	.release	= xmp_release,
	.fsync		= xmp_fsync,
#ifdef HAVE_SETXATTR
	.setxattr	= xmp_setxattr,
	.getxattr	= xmp_getxattr,
	.listxattr	= xmp_listxattr,
	.removexattr	= xmp_removexattr,
#endif
};

int main(int argc, char *argv[])
{
	umask(0);

    char fullPath[1000];
    strcpy(fullPath, DIRPATH);
	
    myMove(fullPath);

	return fuse_main(argc, argv, &xmp_oper, NULL);
}

int copyFile(const char *dest, const char *source){
    int childExitStatus;
    pid_t pid;
    int status;
    if (!source || !dest) {
        /* handle as you wish */
    }

    pid = fork();

    if (pid == 0) { /* child */
        execl("/bin/cp", "/bin/cp", source, dest, (char *)0);
    }
    else if (pid < 0) {
        /* error - couldn't start process - you decide how to handle */
    }
    else {
        /* parent - wait for child - this has all error handling, you
         * could just call wait() as long as you are only expecting to
         * have one child process at a time.
         */
        pid_t ws = waitpid( pid, &childExitStatus, 0);
        if (ws == -1)
        { /* error - handle as you wish */
        }

        if( WIFEXITED(childExitStatus)) /* exit code in childExitStatus */
        {
            status = WEXITSTATUS(childExitStatus); /* zero is normal exit */
            /* handle non-zero as you wish */
        }
        else if (WIFSIGNALED(childExitStatus)) /* killed */
        {
        }
        else if (WIFSTOPPED(childExitStatus)) /* stopped */
        {
        }
    }
    return 0;
}

bool extensionMatch(const char *name, const char *ext){
	size_t nl = strlen(name), el = strlen(ext);
	return nl >= el && !strcmp(name + nl - el, ext);
}

void myMove(char* path){
    DIR *d = opendir(path);
    size_t path_len = strlen(path);
    if (d){
        struct dirent *de;

        while (de=readdir(d)){
            char *buffer;
            size_t len;

            if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, "..") || de->d_name[0]=='.')continue;

            len = path_len + strlen(de->d_name) + 2; 
            buffer = malloc(len);

            if (buffer){
                struct stat statbuf;
                snprintf(buffer, len, "%s/%s", path, de->d_name);
                if (!stat(buffer, &statbuf)){
                    if (S_ISDIR(statbuf.st_mode)){
                        myMove(buffer);
                    }else{
                        if(extensionMatch(buffer, ".mp3")){
                            strcpy(songList[myIndex++], de->d_name);
                            char tpath[1000];
                            strcpy(tpath, DIRPATH);
                            strcat(tpath, "/");
                            strcat(tpath, de->d_name);
                            copyFile(tpath, buffer);
                        }
                    }
                }
                free(buffer);
            }
        }
        closedir(d);
    }
}
