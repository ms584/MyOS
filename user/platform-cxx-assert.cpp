/* GCC's exact target assertion ABI; violations are fatal, never fake success. */
#include <bits/c++config.h>
#include <unistd.h>
#include <stdlib.h>
namespace std {
[[noreturn]] void __glibcxx_assert_fail(const char *file,int line,const char *function,
                                      const char *condition) noexcept {
    (void)file;(void)line;(void)function;
    const char message[]="native GCC precondition failed: ";
    write(2,message,sizeof(message)-1);
    if(condition) { size_t n=0;while(n<256 && condition[n])++n;write(2,condition,n); }
    write(2,"\n",1);abort();
}
}
