/*
 * vcd-h1.cpp
 *
 *  Created on: Jul 31, 2013
 *      Author: tejblum
 */
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#include <vector>
#include <stdexcept>
#include <algorithm>
#include <string>
#include <cstring>

#include "vcd-h1.h"

class fdholder {
 public:
  explicit fdholder(int d) : fd(d) {}
  ~fdholder() { close(fd); }

  operator int() { return fd; }

 private:
  int fd;
  fdholder& operator=(const fdholder&);
};

const char *read_file(const char *fn, blob_type cn)
{
    cn->data.clear();
    fdholder fd(open(fn, O_RDONLY));
    if (fd == -1)
        return "can not open dictionary";
    struct stat st;
    if (fstat(fd, &st) == -1)
        return "dictionary fstat error";
    cn->data.resize(st.st_size);
    if (read(fd, &cn->data[0], cn->data.size()) != (ssize_t)cn->data.size())
        return "dictionary read error";
    return 0;
}



