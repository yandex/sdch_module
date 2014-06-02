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

void read_file(const char *fn, std::vector<char> &cn)
{
    cn.clear();
    fdholder fd(open(fn, O_RDONLY));
    if (fd == -1)
        throw std::runtime_error(std::string("can not open: ") + fn);
    struct stat st;
    if (fstat(fd, &st) == -1)
        throw std::runtime_error("fstat");
    cn.resize(st.st_size);
    if (read(fd, &cn[0], cn.size()) != (ssize_t)cn.size())
        throw std::runtime_error("read");
}

std::vector<char>::const_iterator get_dict_payload(const std::vector<char> &dict)
{
	std::vector<char>::const_iterator nl = dict.begin();
	while (nl < dict.end()) {
		if (*nl == '\n')
			return nl+1;
		nl = find(nl, dict.end(), '\n');
		if (nl == dict.end())
			return nl;
		++nl;
	}
	return dict.end();
}

int get_hashed_dict(const unsigned char *fn, hashed_dictionary_p *d)
{
	try {
		hashed_dictionary_s *h = new hashed_dictionary_s;
		read_file((const char *)fn, h->dict);
		h->dict_payload = &*get_dict_payload(h->dict);
		h->hashed_dict.reset(new open_vcdiff::HashedDictionary(h->dict_payload, &*h->dict.end()-h->dict_payload));
		h->hashed_dict->Init();
		*d = h;
		return 0;
	} catch (...) {
		return 1;
	}
}

void *get_dictionary_begin(hashed_dictionary_p d)
{
	return &d->dict[0];
}

size_t get_dictionary_size(hashed_dictionary_p d)
{
	return d->dict.size();
}

vcd_encoder_s::vcd_encoder_s(writerfunc writer, void *cookie) :
		outstr(writer, cookie)
{
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

void get_vcd_encoder(hashed_dictionary_p d, writerfunc writer, void *cookie, vcd_encoder_p *e) {
	vcd_encoder_s *en = new vcd_encoder_s(writer, cookie);
	en->enc.reset(new open_vcdiff::VCDiffStreamingEncoder(d->hashed_dict.get(),
        open_vcdiff::VCD_FORMAT_INTERLEAVED | open_vcdiff::VCD_FORMAT_CHECKSUM, 0));
	en->enc.get()->StartEncodingToInterface(&en->outstr);
	*e = en;
}
