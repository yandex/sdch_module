#ifndef VCD_H1_H_
#define VCD_H1_H_

#include "pzlib.h"
#include "storage.h"

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
	std::unique_ptr<open_vcdiff::HashedDictionary> hashed_dict;
};
struct vcd_encoder_s {
	struct pz ph;
	std::unique_ptr<open_vcdiff::VCDiffStreamingEncoder> enc;
	NgOuStr outstr;

	vcd_encoder_s(void *c);
};

extern "C" {
#endif

const char *read_file(const char *fn, blob_type cn);
int get_hashed_dict(const char *dictbegin, const char *dictend, int quasi, hashed_dictionary_p *d);

void get_vcd_encoder(hashed_dictionary_p d, void *cookie, vcd_encoder_p *e);

#ifdef __cplusplus
}
#endif

#endif /* VCD_H1_H_ */
