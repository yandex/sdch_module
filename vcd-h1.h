#ifndef VCD_H1_H_
#define VCD_H1_H_

#include "pzlib.h"
#include "storage.h"

struct hashed_dictionary_s;
struct vcd_encoder_s;

typedef struct hashed_dictionary_s* hashed_dictionary_p;
typedef struct vcd_encoder_s *vcd_encoder_p;

#ifdef __cplusplus

#include <memory>

namespace open_vcdiff {
class HashedDictionary;
}

struct hashed_dictionary_s {
	std::unique_ptr<open_vcdiff::HashedDictionary> hashed_dict;
};

extern "C" {
#endif

const char *read_file(const char *fn, blob_type cn);
int get_hashed_dict(const char *dictbegin, const char *dictend, int quasi, hashed_dictionary_p *d);


#ifdef __cplusplus
}
#endif

#endif /* VCD_H1_H_ */
