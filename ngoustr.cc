#include "ngoustr.h"

void NgOuStr::clear()
{
    cursize = 0;
}

void NgOuStr::push_back(char c)
{
    append(&c, 1);
}

size_t NgOuStr::size() const
{
    return cursize;
}

void NgOuStr::ReserveAdditionalBytes(size_t)
{
}

NgOuStr::NgOuStr(writerfunc *w, void *c) :
    cursize(0),
    writer(w),
    cookie(c)
{
}

open_vcdiff::OutputStringInterface& NgOuStr::append(const char *s, size_t n)
{
    writer(cookie, s, n);
    cursize += n;
    return *this;
}
