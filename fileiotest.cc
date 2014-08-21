/*
 * Copyright (C) 2014 Cloudius Systems, Ltd.
 */

#include <algorithm>
#include "reactor.hh"

struct file_test {
    file_test(file&& f) : f(std::move(f)) {}
    file f;
    semaphore sem = { 0 };
};

int main(int ac, char** av) {
    static constexpr auto max = 10000;
    the_reactor.open_file_dma("testfile.tmp").then([] (file f) {
        auto ft = new file_test{std::move(f)};
        for (size_t i = 0; i < 10000; ++i) {
            auto wbuf = allocate_aligned_buffer<unsigned char>(4096, 4096);
            std::fill(wbuf.get(), wbuf.get() + 4096, i);
            auto wb = wbuf.get();
            ft->f.dma_write(i * 4096, wb, 4096).then(
                    [ft, i, wbuf = std::move(wbuf)] (size_t ret) mutable {
                assert(ret == 4096);
                auto rbuf = allocate_aligned_buffer<unsigned char>(4096, 4096);
                auto rb = rbuf.get();
                ft->f.dma_read(i * 4096, rb, 4096).then(
                        [ft, i, rbuf = std::move(rbuf), wbuf = std::move(wbuf)] (size_t) mutable {
                    bool eq = std::equal(rbuf.get(), rbuf.get() + 4096, wbuf.get());
                    assert(eq);
                    ft->sem.signal(1);
                });
            });
        }
        ft->sem.wait(max).then([ft] {
            std::cout << "done\n";
            delete ft;
            ::exit(0);
        });
    });
    the_reactor.run();
    return 0;
}


