/* Extract files from a tar archive.

   Copyright (C) 1988, 1992, 1993, 1994, 1996, 1997, 1998, 1999, 2000,
   2001, 2003, 2004, 2005, 2006, 2007, 2010 Free Software Foundation, Inc.

   Written by John Gilmore, on 1985-11-19.

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 3, or (at your option) any later
   version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
   Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

#include <system.h>
#include <quotearg.h>
#include <errno.h>
#include <priv-set.h>
#include <utimens.h>

#include "common.h"

static bool we_are_root;	/* true if our effective uid == 0 */
static mode_t newdir_umask;	/* umask when creating new directories */
static mode_t current_umask;	/* current umask (which is set to 0 if -p) */

#define ALL_MODE_BITS ((mode_t) ~ (mode_t) 0)

#if ! HAVE_FCHMOD && ! defined fchmod
# define fchmod(fd, mode) (errno = ENOSYS, -1)
#endif
#if ! HAVE_FCHOWN && ! defined fchown
# define fchown(fd, uid, gid) (errno = ENOSYS, -1)
#endif

/* Return true if an error number ERR means the system call is
   supported in this case.  */
static bool
implemented (int err)
{
  return ! (err == ENOSYS
	    || err == ENOTSUP
	    || (EOPNOTSUPP != ENOTSUP && err == EOPNOTSUPP));
}

/* List of directories whose statuses we need to extract after we've
   finished extracting their subsidiary files.  If you consider each
   contiguous subsequence of elements of the form [D]?[^D]*, where [D]
   represents an element where AFTER_LINKS is nonzero and [^D]
   represents an element where AFTER_LINKS is zero, then the head
   of the subsequence has the longest name, and each non-head element
   in the prefix is an ancestor (in the directory hierarchy) of the
   preceding element.  */

struct delayed_set_stat
  {
    /* Next directory in list.  */
    struct delayed_set_stat *next;

    /* Metadata for this directory.  */
    dev_t dev;
    ino_t ino;
    mode_t mode; /* The desired mode is MODE & ~ current_umask.  */
    uid_t uid;
    gid_t gid;
    struct timespec atime;
    struct timespec mtime;

    /* An estimate of the directory's current mode, along with a mask
       specifying which bits of this estimate are known to be correct.
       If CURRENT_MODE_MASK is zero, CURRENT_MODE's value doesn't
       matter.  */
    mode_t current_mode;
    mode_t current_mode_mask;

    /* This directory is an intermediate directory that was created
       as an ancestor of some other directory; it was not mentioned
       in the archive, so do not set its uid, gid, atime, or mtime,
       and don't alter its mode outside of MODE_RWX.  */
    bool interdir;

    /* Whether symbolic links should be followed when accessing the
       directory.  */
    int atflag;

    /* Do not set the status of this directory until after delayed
       links are created.  */
    bool after_links;

    /* Directory that the name is relative to.  */
    int change_dir;

    /* Length and contents of name.  */
    size_t file_name_len;
    char file_name[1];
  };

static struct delayed_set_stat *delayed_set_stat_head;

/* List of links whose creation we have delayed.  */
struct delayed_link
  {
    /* The next delayed link in the list.  */
    struct delayed_link *next;

    /* The device, inode number and ctime of the placeholder.  Use
       ctime, not mtime, to make false matches less likely if some
       other process removes the placeholder.  */
    dev_t dev;
    ino_t ino;
    struct timespec ctime;

    /* True if the link is symbolic.  */
    bool is_symlink;

    /* The desired metadata, valid only the link is symbolic.  */
    mode_t mode;
    uid_t uid;
    gid_t gid;
    struct timespec atime;
    struct timespec mtime;

    /* The directory that the sources and target are relative to.  */
    int change_dir;

    /* A list of sources for this link.  The sources are all to be
       hard-linked together.  */
    struct string_list *sources;

    /* The desired target of the desired link.  */
    char target[1];
  };

static struct delayed_link *delayed_link_head;

struct string_list
  {
    struct string_list *next;
    char string[1];
  };

/*  Set up to extract files.  */
void
extr_init (void)
{
#if defined(WIN32)
  we_are_root = false;
#else
  we_are_root = geteuid () == 0;
#endif
  same_permissions_option += we_are_root;
  same_owner_option += we_are_root;

  /* Option -p clears the kernel umask, so it does not affect proper
     restoration of file permissions.  New intermediate directories will
     comply with umask at start of program.  */

  newdir_umask = umask (0);
  if (0 < same_permissions_option)
    current_umask = 0;
  else
    {
      umask (newdir_umask);	/* restore the kernel umask */
      current_umask = newdir_umask;
    }
}

/* Use fchmod if possible, fchmodat otherwise.  */
static int
fd_chmod (int fd, char const *file, mode_t mode, int atflag)
{
  if (0 <= fd)
    {
      int result = fchmod (fd, mode);
      if (result == 0 || implemented (errno))
	return result;
    }
  return fchmodat (chdir_fd, file, mode, atflag);
}

/* Use fchown if possible, fchownat otherwise.  */
static int
fd_chown (int fd, char const *file, uid_t uid, gid_t gid, int atflag)
{
  if (0 <= fd)
    {
      int result = fchown (fd, uid, gid);
      if (result == 0 || implemented (errno))
	return result;
    }
  return fchownat (chdir_fd, file, uid, gid, atflag);
}

/* Use fstat if possible, fstatat otherwise.  */
static int
fd_stat (int fd, char const *file, struct stat *st, int atflag)
{
  return (0 <= fd
	  ? fstat (fd, st)
	  : fstatat (chdir_fd, file, st, atflag));
}

/* Set the mode for FILE_NAME to MODE.
   MODE_MASK specifies the bits of MODE that we care about;
   thus if MODE_MASK is zero, do nothing.
   If FD is nonnegative, it is a file descriptor for the file.
   CURRENT_MODE and CURRENT_MODE_MASK specify information known about
   the file's current mode, using the style of struct delayed_set_stat.
   TYPEFLAG specifies the type of the file.
   ATFLAG specifies the flag to use when statting the file.  */
static void
set_mode (char const *file_name,
	  mode_t mode, mode_t mode_mask, int fd,
	  mode_t current_mode, mode_t current_mode_mask,
	  char typeflag, int atflag)
{
  if (((current_mode ^ mode) | ~ current_mode_mask) & mode_mask)
    {
      if (MODE_ALL & ~ mode_mask & ~ current_mode_mask)
	{
	  struct stat st;
	  if (fd_stat (fd, file_name, &st, atflag) != 0)
	    {
	      stat_error (file_name);
	      return;
	    }
	  current_mode = st.st_mode;
	}

      current_mode &= MODE_ALL;
      mode = (current_mode & ~ mode_mask) | (mode & mode_mask);

      if (current_mode != mode)
	{
	  int chmod_errno =
	    fd_chmod (fd, file_name, mode, atflag) == 0 ? 0 : errno;

	  /* On Solaris, chmod may fail if we don't have PRIV_ALL, because
	     setuid-root files would otherwise be a backdoor.  See
	     http://opensolaris.org/jive/thread.jspa?threadID=95826
	     (2009-09-03).  */
	  if (chmod_errno == EPERM && (mode & S_ISUID)
	      && priv_set_restore_linkdir () == 0)
	    {
	      chmod_errno =
		fd_chmod (fd, file_name, mode, atflag) == 0 ? 0 : errno;
	      priv_set_remove_linkdir ();
	    }

	  /* Linux fchmodat does not support AT_SYMLINK_NOFOLLOW, and
	     returns ENOTSUP even when operating on non-symlinks, try
	     again with the flag disabled if it does not appear to be
	     supported and if the file is not a symlink.  This
	     introduces a race, alas.  */
	  if (atflag && typeflag != SYMTYPE && ! implemented (chmod_errno))
	    chmod_errno = fd_chmod (fd, file_name, mode, 0) == 0 ? 0 : errno;

	  if (chmod_errno
	      && (typeflag != SYMTYPE || implemented (chmod_errno)))
	    {
	      errno = chmod_errno;
	      chmod_error_details (file_name, mode);
	    }
	}
    }
}

/* Check time after successfully setting FILE_NAME's time stamp to T.  */
static void
check_time (char const *file_name, struct timespec t)
{
  if (t.tv_sec <= 0)
    WARNOPT (WARN_TIMESTAMP,
	     (0, 0, _("%s: implausibly old time stamp %s"),
	      file_name, tartime (t, true)));
  else if (timespec_cmp (volume_start_time, t) < 0)
    {
      struct timespec now;
      gettime (&now);
      if (timespec_cmp (now, t) < 0)
	{
	  char buf[TIMESPEC_STRSIZE_BOUND];
	  struct timespec diff;
	  diff.tv_sec = t.tv_sec - now.tv_sec;
	  diff.tv_nsec = t.tv_nsec - now.tv_nsec;
	  if (diff.tv_nsec < 0)
	    {
	      diff.tv_nsec += BILLION;
	      diff.tv_sec--;
	    }
	  WARNOPT (WARN_TIMESTAMP,
		   (0, 0, _("%s: time stamp %s is %s s in the future"),
		    file_name, tartime (t, true), code_timespec (diff, buf)));
	}
    }
}

/* Restore stat attributes (owner, group, mode and times) for
   FILE_NAME, using information given in *ST.
   If FD is nonnegative, it is a file descriptor for the file.
   CURRENT_MODE and CURRENT_MODE_MASK specify information known about
   the file's current mode, using the style of struct delayed_set_stat.
   TYPEFLAG specifies the type of the file.
   If INTERDIR, this is an intermediate directory.
   ATFLAG specifies the flag to use when statting the file.  */

static void
set_stat (char const *file_name,
	  struct tar_stat_info const *st,
	  int fd, mode_t current_mode, mode_t current_mode_mask,
	  char typeflag, bool interdir, int atflag)
{
  /* Do the utime before the chmod because some versions of utime are
     broken and trash the modes of the file.  */

  if (! touch_option && ! interdir)
    {
      struct timespec ts[2];
      if (incremental_option)
	ts[0] = st->atime;
      else
	ts[0].tv_nsec = UTIME_OMIT;
      ts[1] = st->mtime;

      if (fdutimensat (fd, chdir_fd, file_name, ts, atflag) == 0)
	{
	  if (incremental_option)
	    check_time (file_name, ts[0]);
	  check_time (file_name, ts[1]);
	}
      else if (typeflag != SYMTYPE || implemented (errno))
	utime_error (file_name);
    }

  if (0 < same_owner_option && ! interdir)
    {
      /* Some systems allow non-root users to give files away.  Once this
	 done, it is not possible anymore to change file permissions.
	 However, setting file permissions now would be incorrect, since
	 they would apply to the wrong user, and there would be a race
	 condition.  So, don't use systems that allow non-root users to
	 give files away.  */
      uid_t uid = st->stat.st_uid;
      gid_t gid = st->stat.st_gid;

      if (fd_chown (fd, file_name, uid, gid, atflag) == 0)
	{
	  /* Changing the owner can clear st_mode bits in some cases.  */
	  if ((current_mode | ~ current_mode_mask) & S_IXUGO)
	    current_mode_mask &= ~ (current_mode & (S_ISUID | S_ISGID));
	}
      else if (typeflag != SYMTYPE || implemented (errno))
	chown_error_details (file_name, uid, gid);
    }

  set_mode (file_name,
	    st->stat.st_mode & ~ current_umask,
	    0 < same_permissions_option && ! interdir ? MODE_ALL : MODE_RWX,
	    fd, current_mode, current_mode_mask, typeflag, atflag);
}

/* For each entry H in the leading prefix of entries in HEAD that do
   not have after_links marked, mark H and fill in its dev and ino
   members.  Assume HEAD && ! HEAD->after_links.  */
static void
mark_after_links (struct delayed_set_stat *head)
{
  struct delayed_set_stat *h = head;

  do
    {
      struct stat st;
      h->after_links = 1;

      if (deref_stat (h->file_name, &st) != 0)
	stat_error (h->file_name);
      else
	{
	  h->dev = st.st_dev;
	  h->ino = st.st_ino;
	}
    }
  while ((h = h->next) && ! h->after_links);
}

/* Remember to restore stat attributes (owner, group, mode and times)
   for the directory FILE_NAME, using information given in *ST,
   once we stop extracting files into that directory.

   If ST is null, merely create a placeholder node for an intermediate
   directory that was created by make_directories.

   NOTICE: this works only if the archive has usual member order, i.e.
   directory, then the files in that directory. Incremental archive have
   somewhat reversed order: first go subdirectories, then all other
   members. To help cope with this case the variable
   delay_directory_restore_option is set by prepare_to_extract.

   If an archive was explicitely created so that its member order is
   reversed, some directory timestamps can be restored incorrectly,
   e.g.:
       tar --no-recursion -cf archive dir dir/file1 foo dir/file2
*/
static void
delay_set_stat (char const *file_name, struct tar_stat_info const *st,
		mode_t current_mode, mode_t current_mode_mask,
		mode_t mode, int atflag)
{
  size_t file_name_len = strlen (file_name);
  struct delayed_set_stat *data =
    xmalloc (offsetof (struct delayed_set_stat, file_name)
	     + file_name_len + 1);
  data->next = delayed_set_stat_head;
  data->mode = mode;
  if (st)
    {
      data->dev = st->stat.st_dev;
      data->ino = st->stat.st_ino;
      data->uid = st->stat.st_uid;
      data->gid = st->stat.st_gid;
      data->atime = st->atime;
      data->mtime = st->mtime;
    }
  data->file_name_len = file_name_len;
  data->current_mode = current_mode;
  data->current_mode_mask = current_mode_mask;
  data->interdir = ! st;
  data->atflag = atflag;
  data->after_links = 0;
  data->change_dir = chdir_current;
  strcpy (data->file_name, file_name);
  delayed_set_stat_head = data;
  if (must_be_dot_or_slash (file_name))
    mark_after_links (data);
}

/* Update the delayed_set_stat info for an intermediate directory
   created within the file name of DIR.  The intermediate directory turned
   out to be the same as this directory, e.g. due to ".." or symbolic
   links.  *DIR_STAT_INFO is the status of the directory.  */
static void
repair_delayed_set_stat (char const *dir,
			 struct stat const *dir_stat_info)
{
  struct delayed_set_stat *data;
  for (data = delayed_set_stat_head;  data;  data = data->next)
    {
      struct stat st;
      if (fstatat (chdir_fd, data->file_name, &st, data->atflag) != 0)
	{
	  stat_error (data->file_name);
	  return;
	}

      if (st.st_dev == dir_stat_info->st_dev
	  && st.st_ino == dir_stat_info->st_ino)
	{
	  data->dev = current_stat_info.stat.st_dev;
	  data->ino = current_stat_info.stat.st_ino;
	  data->mode = current_stat_info.stat.st_mode;
	  data->uid = current_stat_info.stat.st_uid;
	  data->gid = current_stat_info.stat.st_gid;
	  data->atime = current_stat_info.atime;
	  data->mtime = current_stat_info.mtime;
	  data->current_mode = st.st_mode;
	  data->current_mode_mask = ALL_MODE_BITS;
	  data->interdir = false;
	  return;
	}
    }

  ERROR ((0, 0, _("%s: Unexpected inconsistency when making directory"),
	  quotearg_colon (dir)));
}

/* After a file/link/directory creation has failed, see if
   it's because some required directory was not present, and if so,
   create all required directories.  Return zero if all the required
   directories were created, nonzero (issuing a diagnostic) otherwise.
   Set *INTERDIR_MADE if at least one directory was created.  */
static int
make_directories (char *file_name, bool *interdir_made)
{
  char *cursor0 = file_name + FILE_SYSTEM_PREFIX_LEN (file_name);
  char *cursor;	        	/* points into the file name */

  for (cursor = cursor0; *cursor; cursor++)
    {
      mode_t mode;
      mode_t desired_mode;
      int status;

      if (! ISSLASH (*cursor))
	continue;

      /* Avoid mkdir of empty string, if leading or double '/'.  */

      if (cursor == cursor0 || ISSLASH (cursor[-1]))
	continue;

      /* Avoid mkdir where last part of file name is "." or "..".  */

      if (cursor[-1] == '.'
	  && (cursor == cursor0 + 1 || ISSLASH (cursor[-2])
	      || (cursor[-2] == '.'
		  && (cursor == cursor0 + 2 || ISSLASH (cursor[-3])))))
	continue;

      *cursor = '\0';		/* truncate the name there */
      desired_mode = MODE_RWX & ~ newdir_umask;
      mode = desired_mode | (we_are_root ? 0 : MODE_WXUSR);
      status = mkdirat (chdir_fd, file_name, mode);

      if (status == 0)
	{
	  /* Create a struct delayed_set_stat even if
	     mode == desired_mode, because
	     repair_delayed_set_stat may need to update the struct.  */
	  delay_set_stat (file_name,
			  0, mode & ~ current_umask, MODE_RWX,
			  desired_mode, AT_SYMLINK_NOFOLLOW);

	  print_for_mkdir (file_name, cursor - file_name, desired_mode);
	  *interdir_made = true;
	}
      else if (errno == EEXIST)
	status = 0;
      else
	{
	  /* Check whether the desired file exists.  Even when the
	     file exists, mkdir can fail with some errno value E other
	     than EEXIST, so long as E describes an error condition
	     that also applies.  */
	  int e = errno;
	  struct stat st;
	  status = fstatat (chdir_fd, file_name, &st, 0);
	  if (status)
	    {
	      errno = e;
	      mkdir_error (file_name);
	    }
	}

      *cursor = '/';
      if (status)
	return status;
    }

  return 0;
}

/* Return true if FILE_NAME (with status *STP, if STP) is not a
   directory, and has a time stamp newer than (or equal to) that of
   TAR_STAT.  */
static bool
file_newer_p (const char *file_name, struct stat const *stp,
	      struct tar_stat_info *tar_stat)
{
  struct stat st;

  if (!stp)
    {
      if (deref_stat (file_name, &st) != 0)
	{
	  if (errno != ENOENT)
	    {
	      stat_warn (file_name);
	      /* Be safer: if the file exists, assume it is newer.  */
	      return true;
	    }
	  return false;
	}
      stp = &st;
    }

  return (! S_ISDIR (stp->st_mode)
	  && tar_timespec_cmp (tar_stat->mtime, get_stat_mtime (stp)) <= 0);
}

#define RECOVER_NO 0
#define RECOVER_OK 1
#define RECOVER_SKIP 2

/* Attempt repairing what went wrong with the extraction.  Delete an
   already existing file or create missing intermediate directories.
   Return RECOVER_OK if we somewhat increased our chances at a successful
   extraction, RECOVER_NO if there are no chances, and RECOVER_SKIP if the
   caller should skip extraction of that member.  The value of errno is
   properly restored on returning RECOVER_NO.

   If REGULAR, the caller was trying to extract onto a regular file.

   Set *INTERDIR_MADE if an intermediate directory is made as part of
   the recovery process.  */

static int
maybe_recoverable (char *file_name, bool regular, bool *interdir_made)
{
  int e = errno;
  struct stat st;
  struct stat const *stp = 0;

  if (*interdir_made)
    return RECOVER_NO;

  switch (e)
    {
    case ELOOP:
      if (! regular
	  || old_files_option != OVERWRITE_OLD_FILES || dereference_option)
	break;
      if (strchr (file_name, '/'))
	{
	  if (deref_stat (file_name, &st) != 0)
	    break;
	  stp = &st;
	}

      /* The caller tried to open a symbolic link with O_NOFOLLOW.
	 Fall through, treating it as an already-existing file.  */

    case EEXIST:
      /* Remove an old file, if the options allow this.  */

      switch (old_files_option)
	{
	case KEEP_OLD_FILES:
	  return RECOVER_SKIP;

	case KEEP_NEWER_FILES:
	  if (file_newer_p (file_name, stp, &current_stat_info))
	    break;
	  /* FALL THROUGH */

	case DEFAULT_OLD_FILES:
	case NO_OVERWRITE_DIR_OLD_FILES:
	case OVERWRITE_OLD_FILES:
	  if (0 < remove_any_file (file_name, ORDINARY_REMOVE_OPTION))
	    return RECOVER_OK;
	  break;

	case UNLINK_FIRST_OLD_FILES:
	  break;
	}

    case ENOENT:
      /* Attempt creating missing intermediate directories.  */
      if (make_directories (file_name, interdir_made) == 0 && *interdir_made)
	return RECOVER_OK;
      break;

    default:
      /* Just say we can't do anything about it...  */
      break;
    }

  errno = e;
  return RECOVER_NO;
}

/* Fix the statuses of all directories whose statuses need fixing, and
   which are not ancestors of FILE_NAME.  If AFTER_LINKS is
   nonzero, do this for all such directories; otherwise, stop at the
   first directory that is marked to be fixed up only after delayed
   links are applied.  */
static void
apply_nonancestor_delayed_set_stat (char const *file_name, bool after_links)
{
  size_t file_name_len = strlen (file_name);
  bool check_for_renamed_directories = 0;

  while (delayed_set_stat_head)
    {
      struct delayed_set_stat *data = delayed_set_stat_head;
      bool skip_this_one = 0;
      struct stat st;
      mode_t current_mode = data->current_mode;
      mode_t current_mode_mask = data->current_mode_mask;

      check_for_renamed_directories |= data->after_links;

      if (after_links < data->after_links
	  || (data->file_name_len < file_name_len
	      && file_name[data->file_name_len]
	      && (ISSLASH (file_name[data->file_name_len])
		  || ISSLASH (file_name[data->file_name_len - 1]))
	      && memcmp (file_name, data->file_name, data->file_name_len) == 0))
	break;

      chdir_do (data->change_dir);

      if (check_for_renamed_directories)
	{
	  if (fstatat (chdir_fd, data->file_name, &st, data->atflag) != 0)
	    {
	      stat_error (data->file_name);
	      skip_this_one = 1;
	    }
	  else
	    {
	      current_mode = st.st_mode;
	      current_mode_mask = ALL_MODE_BITS;
	      if (! (st.st_dev == data->dev && st.st_ino == data->ino))
		{
		  ERROR ((0, 0,
			  _("%s: Directory renamed before its status could be extracted"),
			  quotearg_colon (data->file_name)));
		  skip_this_one = 1;
		}
	    }
	}

      if (! skip_this_one)
	{
	  struct tar_stat_info sb;
	  sb.stat.st_mode = data->mode;
	  sb.stat.st_uid = data->uid;
	  sb.stat.st_gid = data->gid;
	  sb.atime = data->atime;
	  sb.mtime = data->mtime;
	  set_stat (data->file_name, &sb,
		    -1, current_mode, current_mode_mask,
		    DIRTYPE, data->interdir, data->atflag);
	}

      delayed_set_stat_head = data->next;
      free (data);
    }
}



/* Extractor functions for various member types */

static int
extract_dir (char *file_name, int typeflag)
{
  int status;
  mode_t mode;
  mode_t current_mode = 0;
  mode_t current_mode_mask = 0;
  int atflag = 0;
  bool interdir_made = false;

  /* Save 'root device' to avoid purging mount points. */
  if (one_file_system_option && root_device == 0)
    {
      struct stat st;

      if (fstatat (chdir_fd, ".", &st, 0) != 0)
	stat_diag (".");
      else
	root_device = st.st_dev;
    }

  if (incremental_option)
    /* Read the entry and delete files that aren't listed in the archive.  */
    purge_directory (file_name);
  else if (typeflag == GNUTYPE_DUMPDIR)
    skip_member ();

  /* If ownership or permissions will be restored later, create the
     directory with restrictive permissions at first, so that in the
     meantime processes owned by other users do not inadvertently
     create files under this directory that inherit the wrong owner,
     group, or permissions from the directory.  If not root, though,
     make the directory writeable and searchable at first, so that
     files can be created under it.  */
  mode = ((current_stat_info.stat.st_mode
	   & (0 < same_owner_option || 0 < same_permissions_option
	      ? S_IRWXU
	      : MODE_RWX))
	  | (we_are_root ? 0 : MODE_WXUSR));

  for (;;)
    {
      status = mkdirat (chdir_fd, file_name, mode);
      if (status == 0)
	{
	  current_mode = mode & ~ current_umask;
	  current_mode_mask = MODE_RWX;
	  atflag = AT_SYMLINK_NOFOLLOW;
	  break;
	}

      if (errno == EEXIST
	  && (interdir_made
	      || old_files_option == DEFAULT_OLD_FILES
	      || old_files_option == OVERWRITE_OLD_FILES))
	{
	  struct stat st;
	  if (deref_stat (file_name, &st) == 0)
	    {
	      current_mode = st.st_mode;
	      current_mode_mask = ALL_MODE_BITS;

	      if (S_ISDIR (current_mode))
		{
		  if (interdir_made)
		    {
		      repair_delayed_set_stat (file_name, &st);
		      return 0;
		    }
		  break;
		}
	    }
	  errno = EEXIST;
	}

      switch (maybe_recoverable (file_name, false, &interdir_made))
	{
	case RECOVER_OK:
	  continue;

	case RECOVER_SKIP:
	  break;

	case RECOVER_NO:
	  if (errno != EEXIST)
	    {
	      mkdir_error (file_name);
	      return 1;
	    }
	  break;
	}
      break;
    }

  if (status == 0
      || old_files_option == DEFAULT_OLD_FILES
      || old_files_option == OVERWRITE_OLD_FILES)
    delay_set_stat (file_name, &current_stat_info,
		    current_mode, current_mode_mask,
		    current_stat_info.stat.st_mode, atflag);
  return status;
}



static int
open_output_file (char const *file_name, int typeflag, mode_t mode,
		  mode_t *current_mode, mode_t *current_mode_mask)
{
  int fd;
  bool overwriting_old_files = old_files_option == OVERWRITE_OLD_FILES;
  int openflag = (O_WRONLY | O_BINARY | O_CLOEXEC | O_NOCTTY | O_NONBLOCK
		  | O_CREAT
		  | (overwriting_old_files
		     ? O_TRUNC | (dereference_option ? 0 : O_NOFOLLOW)
		     : O_EXCL));

  if (typeflag == CONTTYPE)
    {
      static int conttype_diagnosed;

      if (!conttype_diagnosed)
	{
	  conttype_diagnosed = 1;
	  WARNOPT (WARN_CONTIGUOUS_CAST,
		   (0, 0, _("Extracting contiguous files as regular files")));
	}
    }

  /* If O_NOFOLLOW is needed but does not work, check for a symlink
     separately.  There's a race condition, but that cannot be avoided
     on hosts lacking O_NOFOLLOW.  */
  if (! O_NOFOLLOW && overwriting_old_files && ! dereference_option)
    {
      struct stat st;
      if (fstatat (chdir_fd, file_name, &st, AT_SYMLINK_NOFOLLOW) == 0
	  && S_ISLNK (st.st_mode))
	{
	  errno = ELOOP;
	  return -1;
	}
    }

  fd = openat (chdir_fd, file_name, openflag, mode);
  if (0 <= fd)
    {
      if (overwriting_old_files)
	{
	  struct stat st;
	  if (fstat (fd, &st) != 0)
	    {
	      int e = errno;
	      close (fd);
	      errno = e;
	      return -1;
	    }
	  if (! S_ISREG (st.st_mode))
	    {
	      close (fd);
	      errno = EEXIST;
	      return -1;
	    }
	  *current_mode = st.st_mode;
	  *current_mode_mask = ALL_MODE_BITS;
	}
      else
	{
	  *current_mode = mode & ~ current_umask;
	  *current_mode_mask = MODE_RWX;
	}
    }

  return fd;
}

static int
extract_file (char *file_name, int typeflag)
{
  int fd;
  off_t size;
  union block *data_block;
  int status;
  size_t count;
  size_t written;
  bool interdir_made = false;
  mode_t mode = (current_stat_info.stat.st_mode & MODE_RWX
		 & ~ (0 < same_owner_option ? S_IRWXG | S_IRWXO : 0));
  mode_t current_mode = 0;
  mode_t current_mode_mask = 0;

  if (to_stdout_option)
    fd = STDOUT_FILENO;
#if !defined(WIN32)
  else if (to_command_option)
    {
      fd = sys_exec_command (file_name, 'f', &current_stat_info);
      if (fd < 0)
	{
	  skip_member ();
	  return 0;
	}
    }
#endif
  else
    {
      while ((fd = open_output_file (file_name, typeflag,
#if !defined(WIN32)
				      mode,
#else
/* Make sure the file opened is always writable, or setting utime
 * will fail.  set_stat will make the mode right later.
 */
				      (mode | S_IWUSR),
#endif
				     &current_mode, &current_mode_mask))
	     < 0)
	{
	  int recover = maybe_recoverable (file_name, true, &interdir_made);
	  if (recover != RECOVER_OK)
	    {
	      skip_member ();
	      if (recover == RECOVER_SKIP)
		return 0;
	      open_error (file_name);
	      return 1;
	    }
	}
    }

  mv_begin_read (&current_stat_info);
  if (current_stat_info.is_sparse)
    sparse_extract_file (fd, &current_stat_info, &size);
  else
    for (size = current_stat_info.stat.st_size; size > 0; )
      {
	mv_size_left (size);

	/* Locate data, determine max length writeable, write it,
	   block that we have used the data, then check if the write
	   worked.  */

	data_block = find_next_block ();
	if (! data_block)
	  {
	    ERROR ((0, 0, _("Unexpected EOF in archive")));
	    break;		/* FIXME: What happens, then?  */
	  }

	written = available_space_after (data_block);

	if (written > size)
	  written = size;
	errno = 0;
	count = full_write (fd, data_block->buffer, written);
	size -= written;

	set_next_block_after ((union block *)
			      (data_block->buffer + written - 1));
	if (count != written)
	  {
	    if (!to_command_option)
	      write_error_details (file_name, count, written);
	    /* FIXME: shouldn't we restore from backup? */
	    break;
	  }
      }

  skip_file (size);

  mv_end ();

  /* If writing to stdout, don't try to do anything to the filename;
     it doesn't exist, or we don't want to touch it anyway.  */

  if (to_stdout_option)
    return 0;

  if (! to_command_option)
    set_stat (file_name, &current_stat_info, fd,
	      current_mode, current_mode_mask, typeflag, false,
	      (old_files_option == OVERWRITE_OLD_FILES
	       ? 0 : AT_SYMLINK_NOFOLLOW));

  status = close (fd);
  if (status < 0)
    close_error (file_name);

#if !defined(WIN32)
  if (to_command_option)
    sys_wait_command ();
#endif

  return status;
}

/* Create a placeholder file with name FILE_NAME, which will be
   replaced after other extraction is done by a symbolic link if
   IS_SYMLINK is true, and by a hard link otherwise.  Set
   *INTERDIR_MADE if an intermediate directory is made in the
   process.  */

static int
create_placeholder_file (char *file_name, bool is_symlink, bool *interdir_made)
{
  int fd;
  struct stat st;

  while ((fd = openat (chdir_fd, file_name, O_WRONLY | O_CREAT | O_EXCL, 0)) < 0)
    {
      switch (maybe_recoverable (file_name, false, interdir_made))
	{
	case RECOVER_OK:
	  continue;

	case RECOVER_SKIP:
	  return 0;

	case RECOVER_NO:
	  open_error (file_name);
	  return -1;
	}
      }

  if (fstat (fd, &st) != 0)
    {
      stat_error (file_name);
      close (fd);
    }
  else if (close (fd) != 0)
    close_error (file_name);
  else
    {
      struct delayed_set_stat *h;
      struct delayed_link *p =
	xmalloc (offsetof (struct delayed_link, target)
		 + strlen (current_stat_info.link_name)
		 + 1);
      p->next = delayed_link_head;
      delayed_link_head = p;
      p->dev = st.st_dev;
      p->ino = st.st_ino;
      p->ctime = get_stat_ctime (&st);
      p->is_symlink = is_symlink;
      if (is_symlink)
	{
	  p->mode = current_stat_info.stat.st_mode;
	  p->uid = current_stat_info.stat.st_uid;
	  p->gid = current_stat_info.stat.st_gid;
	  p->atime = current_stat_info.atime;
	  p->mtime = current_stat_info.mtime;
	}
      p->change_dir = chdir_current;
      p->sources = xmalloc (offsetof (struct string_list, string)
			    + strlen (file_name) + 1);
      p->sources->next = 0;
      strcpy (p->sources->string, file_name);
      strcpy (p->target, current_stat_info.link_name);

      h = delayed_set_stat_head;
      if (h && ! h->after_links
	  && strncmp (file_name, h->file_name, h->file_name_len) == 0
	  && ISSLASH (file_name[h->file_name_len])
	  && (last_component (file_name) == file_name + h->file_name_len + 1))
	mark_after_links (h);

      return 0;
    }

  return -1;
}

static int
extract_link (char *file_name, int typeflag)
{
  bool interdir_made = false;
  char const *link_name;
  int rc;

  link_name = current_stat_info.link_name;

  if (! absolute_names_option && contains_dot_dot (link_name))
    return create_placeholder_file (file_name, false, &interdir_made);

  do
    {
      struct stat st1, st2;
      int e;
      int status = linkat (chdir_fd, link_name, chdir_fd, file_name, 0);
      e = errno;

      if (status == 0)
	{
	  struct delayed_link *ds = delayed_link_head;
	  if (ds
	      && fstatat (chdir_fd, link_name, &st1, AT_SYMLINK_NOFOLLOW) == 0)
	    for (; ds; ds = ds->next)
	      if (ds->change_dir == chdir_current
		  && ds->dev == st1.st_dev
		  && ds->ino == st1.st_ino
		  && timespec_cmp (ds->ctime, get_stat_ctime (&st1)) == 0)
		{
		  struct string_list *p =  xmalloc (offsetof (struct string_list, string)
						    + strlen (file_name) + 1);
		  strcpy (p->string, file_name);
		  p->next = ds->sources;
		  ds->sources = p;
		  break;
		}
	  return 0;
	}
      else if ((e == EEXIST && strcmp (link_name, file_name) == 0)
	       || ((fstatat (chdir_fd, link_name, &st1, AT_SYMLINK_NOFOLLOW)
		    == 0)
		   && (fstatat (chdir_fd, file_name, &st2, AT_SYMLINK_NOFOLLOW)
		       == 0)
		   && st1.st_dev == st2.st_dev
		   && st1.st_ino == st2.st_ino))
	return 0;

      errno = e;
    }
  while ((rc = maybe_recoverable (file_name, false, &interdir_made))
	 == RECOVER_OK);

  if (rc == RECOVER_SKIP)
    return 0;
  if (!(incremental_option && errno == EEXIST))
    {
      link_error (link_name, file_name);
      return 1;
    }
  return 0;
}

static int
extract_symlink (char *file_name, int typeflag)
{
#ifdef HAVE_SYMLINK
  bool interdir_made = false;

  if (! absolute_names_option
      && (IS_ABSOLUTE_FILE_NAME (current_stat_info.link_name)
	  || contains_dot_dot (current_stat_info.link_name)))
    return create_placeholder_file (file_name, true, &interdir_made);

  while (symlinkat (current_stat_info.link_name, chdir_fd, file_name) != 0)
    switch (maybe_recoverable (file_name, false, &interdir_made))
      {
      case RECOVER_OK:
	continue;

      case RECOVER_SKIP:
	return 0;

      case RECOVER_NO:
	symlink_error (current_stat_info.link_name, file_name);
	return -1;
      }

  set_stat (file_name, &current_stat_info, -1, 0, 0,
	    SYMTYPE, false, AT_SYMLINK_NOFOLLOW);
  return 0;

#else
  static int warned_once;

  if (!warned_once)
    {
      warned_once = 1;
      WARNOPT (WARN_SYMLINK_CAST,
	       (0, 0,
		_("Attempting extraction of symbolic links as hard links")));
    }
  return extract_link (file_name, typeflag);
#endif
}

#if S_IFCHR || S_IFBLK
static int
extract_node (char *file_name, int typeflag)
{
  bool interdir_made = false;
  mode_t mode = (current_stat_info.stat.st_mode & (MODE_RWX | S_IFBLK | S_IFCHR)
		 & ~ (0 < same_owner_option ? S_IRWXG | S_IRWXO : 0));

  while (mknodat (chdir_fd, file_name, mode, current_stat_info.stat.st_rdev)
	 != 0)
    switch (maybe_recoverable (file_name, false, &interdir_made))
      {
      case RECOVER_OK:
	continue;

      case RECOVER_SKIP:
	return 0;

      case RECOVER_NO:
	mknod_error (file_name);
	return -1;
      }

  set_stat (file_name, &current_stat_info, -1,
	    mode & ~ current_umask, MODE_RWX,
	    typeflag, false, AT_SYMLINK_NOFOLLOW);
  return 0;
}
#endif

#if HAVE_MKFIFO || defined mkfifo
static int
extract_fifo (char *file_name, int typeflag)
{
  bool interdir_made = false;
  mode_t mode = (current_stat_info.stat.st_mode & MODE_RWX
		 & ~ (0 < same_owner_option ? S_IRWXG | S_IRWXO : 0));

  while (mkfifoat (chdir_fd, file_name, mode) != 0)
    switch (maybe_recoverable (file_name, false, &interdir_made))
      {
      case RECOVER_OK:
	continue;

      case RECOVER_SKIP:
	return 0;

      case RECOVER_NO:
	mkfifo_error (file_name);
	return -1;
      }

  set_stat (file_name, &current_stat_info, -1,
	    mode & ~ current_umask, MODE_RWX,
	    typeflag, false, AT_SYMLINK_NOFOLLOW);
  return 0;
}
#endif

static int
extract_volhdr (char *file_name, int typeflag)
{
  skip_member ();
  return 0;
}

static int
extract_failure (char *file_name, int typeflag)
{
  return 1;
}

typedef int (*tar_extractor_t) (char *file_name, int typeflag);



/* Prepare to extract a file. Find extractor function.
   Return zero if extraction should not proceed.  */

static int
prepare_to_extract (char const *file_name, int typeflag, tar_extractor_t *fun)
{
  int rc = 1;

  if (EXTRACT_OVER_PIPE)
    rc = 0;

  /* Select the extractor */
  switch (typeflag)
    {
    case GNUTYPE_SPARSE:
      *fun = extract_file;
      rc = 1;
      break;

    case AREGTYPE:
    case REGTYPE:
    case CONTTYPE:
      /* Appears to be a file.  But BSD tar uses the convention that a slash
	 suffix means a directory.  */
      if (current_stat_info.had_trailing_slash)
	*fun = extract_dir;
      else
	{
	  *fun = extract_file;
	  rc = 1;
	}
      break;

    case SYMTYPE:
      *fun = extract_symlink;
      break;

    case LNKTYPE:
      *fun = extract_link;
      break;

#if S_IFCHR
    case CHRTYPE:
      current_stat_info.stat.st_mode |= S_IFCHR;
      *fun = extract_node;
      break;
#endif

#if S_IFBLK
    case BLKTYPE:
      current_stat_info.stat.st_mode |= S_IFBLK;
      *fun = extract_node;
      break;
#endif

#if HAVE_MKFIFO || defined mkfifo
    case FIFOTYPE:
      *fun = extract_fifo;
      break;
#endif

    case DIRTYPE:
    case GNUTYPE_DUMPDIR:
      *fun = extract_dir;
      if (current_stat_info.is_dumpdir)
	delay_directory_restore_option = true;
      break;

    case GNUTYPE_VOLHDR:
      *fun = extract_volhdr;
      break;

    case GNUTYPE_MULTIVOL:
      ERROR ((0, 0,
	      _("%s: Cannot extract -- file is continued from another volume"),
	      quotearg_colon (current_stat_info.file_name)));
      *fun = extract_failure;
      break;

    case GNUTYPE_LONGNAME:
    case GNUTYPE_LONGLINK:
      ERROR ((0, 0, _("Unexpected long name header")));
      *fun = extract_failure;
      break;

    default:
      WARNOPT (WARN_UNKNOWN_CAST,
	       (0, 0,
		_("%s: Unknown file type `%c', extracted as normal file"),
		quotearg_colon (file_name), typeflag));
      *fun = extract_file;
    }

  /* Determine whether the extraction should proceed */
  if (rc == 0)
    return 0;

  switch (old_files_option)
    {
    case UNLINK_FIRST_OLD_FILES:
      if (!remove_any_file (file_name,
                            recursive_unlink_option ? RECURSIVE_REMOVE_OPTION
                                                      : ORDINARY_REMOVE_OPTION)
	  && errno && errno != ENOENT)
	{
	  unlink_error (file_name);
	  return 0;
	}
      break;

    case KEEP_NEWER_FILES:
      if (file_newer_p (file_name, 0, &current_stat_info))
	{
	  WARNOPT (WARN_IGNORE_NEWER,
		   (0, 0, _("Current %s is newer or same age"),
		    quote (file_name)));
	  return 0;
	}
      break;

    default:
      break;
    }

  return 1;
}

/* Extract a file from the archive.  */
void
extract_archive (void)
{
  char typeflag;
  tar_extractor_t fun;

  fatal_exit_hook = extract_finish;

  set_next_block_after (current_header);

  if (!current_stat_info.file_name[0]
      || (interactive_option
	  && !confirm ("extract", current_stat_info.file_name)))
    {
      skip_member ();
      return;
    }

  /* Print the block from current_header and current_stat.  */
  if (verbose_option)
    print_header (&current_stat_info, current_header, -1);

  /* Restore stats for all non-ancestor directories, unless
     it is an incremental archive.
     (see NOTICE in the comment to delay_set_stat above) */
  if (!delay_directory_restore_option)
    {
      int dir = chdir_current;
      apply_nonancestor_delayed_set_stat (current_stat_info.file_name, 0);
      chdir_do (dir);
    }

  /* Take a safety backup of a previously existing file.  */

  if (backup_option)
    if (!maybe_backup_file (current_stat_info.file_name, 0))
      {
	int e = errno;
	ERROR ((0, e, _("%s: Was unable to backup this file"),
		quotearg_colon (current_stat_info.file_name)));
	skip_member ();
	return;
      }

  /* Extract the archive entry according to its type.  */
  /* KLUDGE */
  typeflag = sparse_member_p (&current_stat_info) ?
                  GNUTYPE_SPARSE : current_header->header.typeflag;

  if (prepare_to_extract (current_stat_info.file_name, typeflag, &fun))
    {
      if (fun && (*fun) (current_stat_info.file_name, typeflag)
	  && backup_option)
	undo_last_backup ();
    }
  else
    skip_member ();

}

/* Extract the links whose final extraction were delayed.  */
static void
apply_delayed_links (void)
{
  struct delayed_link *ds;

  for (ds = delayed_link_head; ds; )
    {
      struct string_list *sources = ds->sources;
      char const *valid_source = 0;

      chdir_do (ds->change_dir);

      for (sources = ds->sources; sources; sources = sources->next)
	{
	  char const *source = sources->string;
	  struct stat st;

	  /* Make sure the placeholder file is still there.  If not,
	     don't create a link, as the placeholder was probably
	     removed by a later extraction.  */
	  if (fstatat (chdir_fd, source, &st, AT_SYMLINK_NOFOLLOW) == 0
	      && st.st_dev == ds->dev
	      && st.st_ino == ds->ino
	      && timespec_cmp (get_stat_ctime (&st), ds->ctime) == 0)
	    {
	      /* Unlink the placeholder, then create a hard link if possible,
		 a symbolic link otherwise.  */
	      if (unlinkat (chdir_fd, source, 0) != 0)
		unlink_error (source);
	      else if (valid_source
		       && (linkat (chdir_fd, valid_source, chdir_fd, source, 0)
			   == 0))
		;
	      else if (!ds->is_symlink)
		{
		  if (linkat (chdir_fd, ds->target, chdir_fd, source, 0) != 0)
		    link_error (ds->target, source);
		}
	      else if (symlinkat (ds->target, chdir_fd, source) != 0)
		symlink_error (ds->target, source);
	      else
		{
		  struct tar_stat_info st1;
		  st1.stat.st_mode = ds->mode;
		  st1.stat.st_uid = ds->uid;
		  st1.stat.st_gid = ds->gid;
		  st1.atime = ds->atime;
		  st1.mtime = ds->mtime;
		  set_stat (source, &st1, -1, 0, 0, SYMTYPE,
			    false, AT_SYMLINK_NOFOLLOW);
		  valid_source = source;
		}
	    }
	}

      for (sources = ds->sources; sources; )
	{
	  struct string_list *next = sources->next;
	  free (sources);
	  sources = next;
	}

      {
	struct delayed_link *next = ds->next;
	free (ds);
	ds = next;
      }
    }

  delayed_link_head = 0;
}

/* Finish the extraction of an archive.  */
void
extract_finish (void)
{
  /* First, fix the status of ordinary directories that need fixing.  */
  apply_nonancestor_delayed_set_stat ("", 0);

  /* Then, apply delayed links, so that they don't affect delayed
     directory status-setting for ordinary directories.  */
  apply_delayed_links ();

  /* Finally, fix the status of directories that are ancestors
     of delayed links.  */
  apply_nonancestor_delayed_set_stat ("", 1);
}

bool
rename_directory (char *src, char *dst)
{
  if (renameat (chdir_fd, src, chdir_fd, dst) != 0)
    {
      int e = errno;
      bool interdir_made;

      switch (e)
	{
	case ENOENT:
	  if (make_directories (dst, &interdir_made) == 0)
	    {
	      if (renameat (chdir_fd, src, chdir_fd, dst) == 0)
		return true;
	      e = errno;
	    }
	  break;

	case EXDEV:
	  /* FIXME: Fall back to recursive copying */

	default:
	  break;
	}

      ERROR ((0, e, _("Cannot rename %s to %s"),
	      quote_n (0, src),
	      quote_n (1, dst)));
      return false;
    }
  return true;
}
