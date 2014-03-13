#ifndef NG_OU_STR_H__
#define NG_OU_STR_H__

#include <google/output_string.h>
#include "pzlib.h"

class NgOuStr : public open_vcdiff::OutputStringInterface {
private:
    size_t cursize;
    void *cookie;
public:
    NgOuStr(void *c);
    ~NgOuStr();
    virtual OutputStringInterface& append(const char* s, size_t n);
    virtual void clear();
    virtual void push_back(char c);
    virtual void ReserveAdditionalBytes(size_t res_arg);
    virtual size_t size() const;
};


#endif
