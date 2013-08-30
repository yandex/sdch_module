#include <err.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#include <vector>
#include <stdexcept>

#include <google/vcencoder.h>

#include "pzlib.h"
#include "ngoustr.h"

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

int main0(int argc, char *argv[])
{
    if (argc != 3)
        errx(1, "usage");

    std::vector<char> src;
    read_file(argv[2], src);
    
    hashed_dictionary_p hd;
    get_hashed_dict(argv[1], &hd);
    
    vcd_encoder_p e;
    get_vcd_encoder(hd, write_fd, make_fd(1), &e);
    vcdwriter(e, &src[0], src.size());
    vcdclose(e);

    return 0;
}

int main(int argc, char *argv[])
{
    try {
        return main0(argc, argv);
    } catch (std::exception &e) {
        fprintf(stderr, "exc: %s\n", e.what());
        return 1;
    }
}
