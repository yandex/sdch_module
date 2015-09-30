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

#include <google/vcencoder.h>

#include "vcd-h1.h"

class fdholder
{
private:
    int fd;
    fdholder &operator =(const fdholder&);
public:
    operator int() {return fd;}
    explicit fdholder(int d): fd(d) {}
    ~fdholder() {close(fd);}
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

const char *get_dict_payload(const char *dictbegin, const char *dictend)
{
	const char *nl = dictbegin;
	while (nl < dictend) {
		if (*nl == '\n')
			return nl+1;
		nl = (const char*)memchr(nl, '\n', dictend-nl);
		if (nl == dictend)
			return nl;
		++nl;
	}
	return dictend;
}

int get_hashed_dict(const char *dictbegin, const char *dictend, int quasi, hashed_dictionary_p *d)
{
	try {
		hashed_dictionary_s *h = new hashed_dictionary_s;
		const char *dict_payload = quasi ? dictbegin : get_dict_payload(dictbegin, dictend);
		h->hashed_dict.reset(new open_vcdiff::HashedDictionary(dict_payload, dictend-dict_payload));
		if (!h->hashed_dict->Init()) {
      delete h;
      return 1;
    }
		*d = h;
		return 0;
	} catch (...) {
		return 1;
	}
}

