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

NgOuStr::NgOuStr(void *c) :
    cursize(0),
    cookie(c)
{
}

NgOuStr::~NgOuStr()
{
    do_close(cookie);
}

open_vcdiff::OutputStringInterface& NgOuStr::append(const char *s, size_t n)
{
    do_write(cookie, s, n);
    cursize += n;
    return *this;
}
