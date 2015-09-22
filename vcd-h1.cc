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
#include "ngoustr.h"

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

static writerfunc vcdwriter;
static closefunc vcdclose;

vcd_encoder_s::vcd_encoder_s(void *cookie) :
		outstr(cookie)
{
        ph.wf = vcdwriter;
        ph.cf = vcdclose;
}

pssize_type vcdwriter(void *cookie, const void *buf, psize_type len)
{
	vcd_encoder_s *e = (vcd_encoder_s*)cookie;
	e->enc->EncodeChunkToInterface((const char *)buf, len, &e->outstr);
	return len;
}

void vcdclose(void *cookie) {
	vcd_encoder_s *e = (vcd_encoder_s*)cookie;
	e->enc->FinishEncodingToInterface(&e->outstr);
	delete e;
}

void get_vcd_encoder(hashed_dictionary_p d, void *cookie, vcd_encoder_p *e) {
	vcd_encoder_s *en = new vcd_encoder_s(cookie);
	en->enc.reset(new open_vcdiff::VCDiffStreamingEncoder(d->hashed_dict.get(),
        open_vcdiff::VCD_FORMAT_INTERLEAVED | open_vcdiff::VCD_FORMAT_CHECKSUM, 0));
	en->enc.get()->StartEncodingToInterface(&en->outstr);
	*e = en;
}
