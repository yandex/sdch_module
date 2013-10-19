#ifndef VCD_H1_H_
#define VCD_H1_H_

#include "pzlib.h"

#ifdef __cplusplus
#include "ngoustr.h"
#endif

struct hashed_dictionary_s;
struct vcd_encoder_s;

typedef struct hashed_dictionary_s* hashed_dictionary_p;
typedef struct vcd_encoder_s *vcd_encoder_p;

#ifdef __cplusplus

#include <memory>

struct hashed_dictionary_s {
	std::vector<char> dict;
	const char *dict_payload;
	std::auto_ptr<open_vcdiff::HashedDictionary> hashed_dict;
};
struct vcd_encoder_s {
	std::auto_ptr<open_vcdiff::VCDiffStreamingEncoder> enc;
	NgOuStr outstr;

	vcd_encoder_s(writerfunc wf, void *c);
};
void read_file(const char *fn, std::vector<char> &cn);

extern "C" {
#endif

int get_hashed_dict(const char *fn, hashed_dictionary_p *d);
void *get_dictionary_begin(hashed_dictionary_p d);
size_t get_dictionary_size(hashed_dictionary_p d);

void get_vcd_encoder(hashed_dictionary_p d, writerfunc writer, void *cookie, vcd_encoder_p *e);
extern writerfunc vcdwriter;
extern closefunc vcdclose;

#ifdef __cplusplus
}
#endif

#endif /* VCD_H1_H_ */
