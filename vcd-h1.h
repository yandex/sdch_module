#ifndef VCD_H1_H_
#define VCD_H1_H_

#include <memory>

#include "storage.h"

struct hashed_dictionary_s;

typedef struct hashed_dictionary_s* hashed_dictionary_p;

namespace open_vcdiff {
class HashedDictionary;
}

struct hashed_dictionary_s {
	std::unique_ptr<open_vcdiff::HashedDictionary> hashed_dict;
};

const char *read_file(const char *fn, blob_type cn);
int get_hashed_dict(const char* dictbegin,
                    const char* dictend,
                    int quasi,
                    hashed_dictionary_p* d);


#endif /* VCD_H1_H_ */
