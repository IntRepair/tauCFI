/* A substitute for ISO C99 <wctype.h>, for platforms that lack it.

   Copyright (C) 2006-2011 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

/* Written by Bruno Haible and Paul Eggert.  */

/*
 * ISO C 99 <wctype.h> for platforms that lack it.
 * <http://www.opengroup.org/susv3xbd/wctype.h.html>
 *
 * iswctype, towctrans, towlower, towupper, wctrans, wctype,
 * wctrans_t, and wctype_t are not yet implemented.
 */

#ifndef _GL_WCTYPE_H

#if __GNUC__ >= 3
@PRAGMA_SYSTEM_HEADER@
#endif
@PRAGMA_COLUMNS@

#if @HAVE_WINT_T@
/* Solaris 2.5 has a bug: <wchar.h> must be included before <wctype.h>.
   Tru64 with Desktop Toolkit C has a bug: <stdio.h> must be included before
   <wchar.h>.
   BSD/OS 4.0.1 has a bug: <stddef.h>, <stdio.h> and <time.h> must be
   included before <wchar.h>.  */
# include <stddef.h>
# include <stdio.h>
# include <time.h>
# include <wchar.h>
#endif

/* Include the original <wctype.h> if it exists.
   BeOS 5 has the functions but no <wctype.h>.  */
/* The include_next requires a split double-inclusion guard.  */
#if @HAVE_WCTYPE_H@
# @INCLUDE_NEXT@ @NEXT_WCTYPE_H@
#endif

#ifndef _GL_WCTYPE_H
#define _GL_WCTYPE_H

/* The definitions of _GL_FUNCDECL_RPL etc. are copied here.  */

/* The definition of _GL_WARN_ON_USE is copied here.  */

/* Define wint_t and WEOF.  (Also done in wchar.in.h.)  */
#if !@HAVE_WINT_T@ && !defined wint_t
# define wint_t int
# ifndef WEOF
#  define WEOF -1
# endif
#else
# ifndef WEOF
#  define WEOF ((wint_t) -1)
# endif
#endif


/* FreeBSD 4.4 to 4.11 has <wctype.h> but lacks the functions.
   Linux libc5 has <wctype.h> and the functions but they are broken.
   Assume all 11 functions (all isw* except iswblank) are implemented the
   same way, or not at all.  */
#if ! @HAVE_ISWCNTRL@ || @REPLACE_ISWCNTRL@

/* IRIX 5.3 has macros but no functions, its isw* macros refer to an
   undefined variable _ctmp_ and to <ctype.h> macros like _P, and they
   refer to system functions like _iswctype that are not in the
   standard C library.  Rather than try to get ancient buggy
   implementations like this to work, just disable them.  */
# undef iswalnum
# undef iswalpha
# undef iswblank
# undef iswcntrl
# undef iswdigit
# undef iswgraph
# undef iswlower
# undef iswprint
# undef iswpunct
# undef iswspace
# undef iswupper
# undef iswxdigit
# undef towlower
# undef towupper

/* Linux libc5 has <wctype.h> and the functions but they are broken.  */
# if @REPLACE_ISWCNTRL@
#  if !(defined __cplusplus && defined GNULIB_NAMESPACE)
#   define iswalnum rpl_iswalnum
#   define iswalpha rpl_iswalpha
#   define iswblank rpl_iswblank
#   define iswcntrl rpl_iswcntrl
#   define iswdigit rpl_iswdigit
#   define iswgraph rpl_iswgraph
#   define iswlower rpl_iswlower
#   define iswprint rpl_iswprint
#   define iswpunct rpl_iswpunct
#   define iswspace rpl_iswspace
#   define iswupper rpl_iswupper
#   define iswxdigit rpl_iswxdigit
#   define towlower rpl_towlower
#   define towupper rpl_towupper
#  endif
# endif

static inline int
# if @REPLACE_ISWCNTRL@
rpl_iswalnum
# else
iswalnum
# endif
         (wint_t wc)
{
  return ((wc >= '0' && wc <= '9')
          || ((wc & ~0x20) >= 'A' && (wc & ~0x20) <= 'Z'));
}

static inline int
# if @REPLACE_ISWCNTRL@
rpl_iswalpha
# else
iswalpha
# endif
         (wint_t wc)
{
  return (wc & ~0x20) >= 'A' && (wc & ~0x20) <= 'Z';
}

static inline int
# if @REPLACE_ISWCNTRL@
rpl_iswblank
# else
iswblank
# endif
         (wint_t wc)
{
  return wc == ' ' || wc == '\t';
}

static inline int
# if @REPLACE_ISWCNTRL@
rpl_iswcntrl
# else
iswcntrl
# endif
        (wint_t wc)
{
  return (wc & ~0x1f) == 0 || wc == 0x7f;
}

static inline int
# if @REPLACE_ISWCNTRL@
rpl_iswdigit
# else
iswdigit
# endif
         (wint_t wc)
{
  return wc >= '0' && wc <= '9';
}

static inline int
# if @REPLACE_ISWCNTRL@
rpl_iswgraph
# else
iswgraph
# endif
         (wint_t wc)
{
  return wc >= '!' && wc <= '~';
}

static inline int
# if @REPLACE_ISWCNTRL@
rpl_iswlower
# else
iswlower
# endif
         (wint_t wc)
{
  return wc >= 'a' && wc <= 'z';
}

static inline int
# if @REPLACE_ISWCNTRL@
rpl_iswprint
# else
iswprint
# endif
         (wint_t wc)
{
  return wc >= ' ' && wc <= '~';
}

static inline int
# if @REPLACE_ISWCNTRL@
rpl_iswpunct
# else
iswpunct
# endif
         (wint_t wc)
{
  return (wc >= '!' && wc <= '~'
          && !((wc >= '0' && wc <= '9')
               || ((wc & ~0x20) >= 'A' && (wc & ~0x20) <= 'Z')));
}

static inline int
# if @REPLACE_ISWCNTRL@
rpl_iswspace
# else
iswspace
# endif
         (wint_t wc)
{
  return (wc == ' ' || wc == '\t'
          || wc == '\n' || wc == '\v' || wc == '\f' || wc == '\r');
}

static inline int
# if @REPLACE_ISWCNTRL@
rpl_iswupper
# else
iswupper
# endif
         (wint_t wc)
{
  return wc >= 'A' && wc <= 'Z';
}

static inline int
# if @REPLACE_ISWCNTRL@
rpl_iswxdigit
# else
iswxdigit
# endif
          (wint_t wc)
{
  return ((wc >= '0' && wc <= '9')
          || ((wc & ~0x20) >= 'A' && (wc & ~0x20) <= 'F'));
}

static inline wint_t
# if @REPLACE_ISWCNTRL@
rpl_towlower
# else
towlower
# endif
         (wint_t wc)
{
  return (wc >= 'A' && wc <= 'Z' ? wc - 'A' + 'a' : wc);
}

static inline wint_t
# if @REPLACE_ISWCNTRL@
rpl_towupper
# else
towupper
# endif
         (wint_t wc)
{
  return (wc >= 'a' && wc <= 'z' ? wc - 'a' + 'A' : wc);
}

#elif ! @HAVE_ISWBLANK@ || @REPLACE_ISWBLANK@
/* Only the iswblank function is missing.  */

# if @REPLACE_ISWBLANK@
#  if !(defined __cplusplus && defined GNULIB_NAMESPACE)
#   define iswblank rpl_iswblank
#  endif
_GL_FUNCDECL_RPL (iswblank, int, (wint_t wc));
# else
_GL_FUNCDECL_SYS (iswblank, int, (wint_t wc));
# endif

#endif

#if defined __MINGW32__

/* On native Windows, wchar_t is uint16_t, and wint_t is uint32_t.
   The functions towlower and towupper are implemented in the MSVCRT library
   to take a wchar_t argument and return a wchar_t result.  mingw declares
   these functions to take a wint_t argument and return a wint_t result.
   This means that:
   1. When the user passes an argument outside the range 0x0000..0xFFFF, the
      function will look only at the lower 16 bits.  This is allowed according
      to POSIX.
   2. The return value is returned in the lower 16 bits of the result register.
      The upper 16 bits are random: whatever happened to be in that part of the
      result register.  We need to fix this by adding a zero-extend from
      wchar_t to wint_t after the call.  */

static inline wint_t
rpl_towlower (wint_t wc)
{
  return (wint_t) (wchar_t) towlower (wc);
}
# if !(defined __cplusplus && defined GNULIB_NAMESPACE)
#  define towlower rpl_towlower
# endif

static inline wint_t
rpl_towupper (wint_t wc)
{
  return (wint_t) (wchar_t) towupper (wc);
}
# if !(defined __cplusplus && defined GNULIB_NAMESPACE)
#  define towupper rpl_towupper
# endif

#endif /* __MINGW32__ */

#if @REPLACE_ISWCNTRL@
_GL_CXXALIAS_RPL (iswalnum, int, (wint_t wc));
_GL_CXXALIAS_RPL (iswalpha, int, (wint_t wc));
_GL_CXXALIAS_RPL (iswblank, int, (wint_t wc));
_GL_CXXALIAS_RPL (iswcntrl, int, (wint_t wc));
_GL_CXXALIAS_RPL (iswdigit, int, (wint_t wc));
_GL_CXXALIAS_RPL (iswgraph, int, (wint_t wc));
_GL_CXXALIAS_RPL (iswlower, int, (wint_t wc));
_GL_CXXALIAS_RPL (iswprint, int, (wint_t wc));
_GL_CXXALIAS_RPL (iswpunct, int, (wint_t wc));
_GL_CXXALIAS_RPL (iswspace, int, (wint_t wc));
_GL_CXXALIAS_RPL (iswupper, int, (wint_t wc));
_GL_CXXALIAS_RPL (iswxdigit, int, (wint_t wc));
#else
_GL_CXXALIAS_SYS (iswalnum, int, (wint_t wc));
_GL_CXXALIAS_SYS (iswalpha, int, (wint_t wc));
# if @REPLACE_ISWBLANK@
_GL_CXXALIAS_RPL (iswblank, int, (wint_t wc));
# else
_GL_CXXALIAS_SYS (iswblank, int, (wint_t wc));
# endif
_GL_CXXALIAS_SYS (iswcntrl, int, (wint_t wc));
_GL_CXXALIAS_SYS (iswdigit, int, (wint_t wc));
_GL_CXXALIAS_SYS (iswgraph, int, (wint_t wc));
_GL_CXXALIAS_SYS (iswlower, int, (wint_t wc));
_GL_CXXALIAS_SYS (iswprint, int, (wint_t wc));
_GL_CXXALIAS_SYS (iswpunct, int, (wint_t wc));
_GL_CXXALIAS_SYS (iswspace, int, (wint_t wc));
_GL_CXXALIAS_SYS (iswupper, int, (wint_t wc));
_GL_CXXALIAS_SYS (iswxdigit, int, (wint_t wc));
#endif
_GL_CXXALIASWARN (iswalnum);
_GL_CXXALIASWARN (iswalpha);
_GL_CXXALIASWARN (iswblank);
_GL_CXXALIASWARN (iswcntrl);
_GL_CXXALIASWARN (iswdigit);
_GL_CXXALIASWARN (iswgraph);
_GL_CXXALIASWARN (iswlower);
_GL_CXXALIASWARN (iswprint);
_GL_CXXALIASWARN (iswpunct);
_GL_CXXALIASWARN (iswspace);
_GL_CXXALIASWARN (iswupper);
_GL_CXXALIASWARN (iswxdigit);

#if @REPLACE_ISWCNTRL@ || defined __MINGW32__
_GL_CXXALIAS_RPL (towlower, wint_t, (wint_t wc));
_GL_CXXALIAS_RPL (towupper, wint_t, (wint_t wc));
#else
_GL_CXXALIAS_SYS (towlower, wint_t, (wint_t wc));
_GL_CXXALIAS_SYS (towupper, wint_t, (wint_t wc));
#endif
_GL_CXXALIASWARN (towlower);
_GL_CXXALIASWARN (towupper);


#endif /* _GL_WCTYPE_H */
#endif /* _GL_WCTYPE_H */
