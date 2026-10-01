/* Standalone target provider for the renderer's explicit no-exception profile. */
#include <new>
#include <stdlib.h>
#ifdef __cpp_exceptions
#error This provider does not implement exception throwing/unwinding
#endif
extern "C" int posix_memalign(void **,size_t,size_t);
namespace std {
const nothrow_t nothrow{};
[[noreturn]] void __throw_bad_alloc(){__builtin_trap();}
[[noreturn]] void __throw_bad_array_new_length(){__builtin_trap();}
[[noreturn]] void __throw_length_error(const char *){__builtin_trap();}
[[noreturn]] void __throw_system_error(int){__builtin_trap();}
}
static void *allocate(size_t size,size_t alignment,bool nullable)
{
    if(!alignment||(alignment&(alignment-1))){if(!nullable)__builtin_trap();return nullptr;}
    if(!size)size=1;
    void *result=nullptr;
    if(alignment<=16)result=malloc(size);
    else if(posix_memalign(&result,alignment,size))result=nullptr;
    if(!result&&!nullable)std::__throw_bad_alloc();
    return result;
}
void *operator new(size_t size){return allocate(size,16,false);}
void *operator new[](size_t size){return allocate(size,16,false);}
void operator delete(void *p) noexcept {free(p);}
void operator delete[](void *p) noexcept {free(p);}
void operator delete(void *p,size_t) noexcept {free(p);}
void operator delete[](void *p,size_t) noexcept {free(p);}
void *operator new(size_t size,const std::nothrow_t &) noexcept {return allocate(size,16,true);}
void *operator new[](size_t size,const std::nothrow_t &) noexcept {return allocate(size,16,true);}
void operator delete(void *p,const std::nothrow_t &) noexcept {free(p);}
void operator delete[](void *p,const std::nothrow_t &) noexcept {free(p);}
void *operator new(size_t size,std::align_val_t alignment){return allocate(size,(size_t)alignment,false);}
void *operator new[](size_t size,std::align_val_t alignment){return allocate(size,(size_t)alignment,false);}
void operator delete(void *p,std::align_val_t) noexcept {free(p);}
void operator delete[](void *p,std::align_val_t) noexcept {free(p);}
void operator delete(void *p,size_t,std::align_val_t) noexcept {free(p);}
void operator delete[](void *p,size_t,std::align_val_t) noexcept {free(p);}
void *operator new(size_t size,std::align_val_t a,const std::nothrow_t &) noexcept {return allocate(size,(size_t)a,true);}
void *operator new[](size_t size,std::align_val_t a,const std::nothrow_t &) noexcept {return allocate(size,(size_t)a,true);}
void operator delete(void *p,std::align_val_t,const std::nothrow_t &) noexcept {free(p);}
void operator delete[](void *p,std::align_val_t,const std::nothrow_t &) noexcept {free(p);}
