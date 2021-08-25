/*
 * Copyright (C) 2006 Tommi Maekitalo
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * is provided AS IS, WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, and
 * NON-INFRINGEMENT.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 *
 */

#include "_dirent.h"
#include <zim/zim.h>
#include "buffer.h"
#include "endian_tools.h"
#include "log.h"
#include <algorithm>
#include <cstring>
#ifdef _WIN32
# include <io.h>
#else
# include <unistd.h>
# define _write(fd, addr, size) if(::write((fd), (addr), (size)) != (ssize_t)(size)) \
{throw std::runtime_error("Error writing");}
#endif

log_define("zim.dirent")

namespace zim {
namespace writer {

Dirent::Dirent()
  : mimeType(0),
    ns(0),
    pathTitle(),
    info(DirentInfo::Direct()),
    removed(false)
{}

// Creator for a "classic" dirent
Dirent::Dirent(char ns, const std::string& path, const std::string& title, uint16_t mimetype)
  : mimeType(mimetype),
    ns(ns),
    pathTitle(path, title),
    info(DirentInfo::Direct()),
    removed(false)
{}

// Creator for a "redirection" dirent
Dirent::Dirent(char ns, const std::string& path, const std::string& title, char targetNs, const std::string& targetPath)
  : mimeType(redirectMimeType),
    ns(ns),
    pathTitle(path, title),
    info(DirentInfo::Redirect(targetNs, targetPath)),
    removed(false)
{}

char Dirent::getRedirectNs() const {
  return info.getRedirect().ns;
}

std::string Dirent::getRedirectPath() const {
  return info.getRedirect().targetPath;
}

void Dirent::write(int out_fd) const
{
  const static char zero = 0;
  union
  {
    char d[16];
    long a;
  } header;
  zim::toLittleEndian(getMimeType(), header.d);
  header.d[2] = 0; // parameter size
  header.d[3] = getNamespace();

  log_debug("title=" << dirent.getTitle() << " title.size()=" << dirent.getTitle().size());

  zim::toLittleEndian(getVersion(), header.d + 4);

  if (isRedirect())
  {
    zim::toLittleEndian(getRedirectIndex().v, header.d + 8);
    _write(out_fd, header.d, 12);
  }
  else
  {
    zim::toLittleEndian(zim::cluster_index_type(getClusterNumber()), header.d + 8);
    zim::toLittleEndian(zim::blob_index_type(getBlobNumber()), header.d + 12);
    _write(out_fd, header.d, 16);
  }

  _write(out_fd, pathTitle.data(), pathTitle.size());
  _write(out_fd, &zero, 1);

}

}
}
