#include <vector>
#include <unordered_map>
#include <utility>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
static void require(bool good) {
    if(!good) { const char fail[]="native GCC containers assertion failed\n";
        write(1,fail,sizeof(fail)-1);abort(); }
}
struct Tracked {
    int *live;unsigned value;
    Tracked(int *p,unsigned v):live(p),value(v){++*live;}
    Tracked(const Tracked &v):live(v.live),value(v.value){++*live;}
    Tracked &operator=(const Tracked &v){value=v.value;return *this;}
    ~Tracked(){--*live;}
};
struct alignas(64) Aligned { unsigned long value; };
struct Collision { size_t operator()(unsigned long n) const {return n%7;} };
static void containers() {
    int live=0;
    {
        std::vector<Tracked> values;
        for(unsigned i=0;i<257;++i)values.emplace_back(&live,i*i);
        require(values.size()==257 && live==257);
        for(unsigned i=0;i<257;++i)require(values[i].value==i*i);
        values.erase(values.begin()+17,values.begin()+29);
        require(values.size()==245 && live==245 && values[17].value==29*29);
        std::vector<Tracked> copy(values);require(live==490);
        std::vector<Tracked> moved(std::move(copy));require(live==490 && moved.size()==245);
        values.clear();require(live==245);
        moved.shrink_to_fit();require(live==245);
        moved.resize(0,moved[0]);require(live==0);
        std::vector<Aligned> aligned(33);
        require(((uintptr_t)aligned.data()%64)==0);
        for(unsigned i=0;i<33;++i)aligned[i].value=i+5;
        aligned.reserve(80);require(((uintptr_t)aligned.data()%64)==0);
        for(unsigned i=0;i<33;++i)require(aligned[i].value==i+5);
    }
    require(live==0);
    std::unordered_map<unsigned long,unsigned long,Collision> map;
    map.max_load_factor(0.7f);
    for(unsigned long i=0;i<513;++i)require(map.emplace(i,i*17+3).second);
    require(map.size()==513 && !map.emplace(7,999).second && map.at(7)==122);
    size_t old=map.bucket_count();map.rehash(old*2+1);
    require(map.bucket_count()>old && map.load_factor()<=map.max_load_factor());
    for(unsigned long i=0;i<513;++i)require(map.find(i)!=map.end() && map.at(i)==i*17+3);
    for(unsigned long i=0;i<513;i+=3)require(map.erase(i)==1 && map.find(i)==map.end());
    require(map.size()==342);
    std::unordered_map<unsigned long,unsigned long,Collision> copy(map),moved(std::move(copy));
    require(moved==map);map.clear();map.swap(moved);require(map.size()==342 && moved.empty());
    map.reserve(2048);
    unsigned long sum=0;for(const auto &item:map)sum+=item.second;
    unsigned long expected=0;for(unsigned long i=0;i<513;++i)if(i%3)expected+=i*17+3;
    require(sum==expected);
}
static void *worker(void *value) { for(int i=0;i<8;++i)containers();return value; }
extern "C" int _start() {
#ifdef CONTAINER_LENGTH_FAULT
    std::vector<unsigned long> value;value.reserve(SIZE_MAX);return 1;
#elif defined(CONTAINER_RANGE_FAULT)
    std::unordered_map<unsigned long,unsigned long,Collision> map;return (int)map.at(123);
#elif defined(CONTAINER_ASSERT_FAULT)
    std::vector<unsigned long> value(1,7);volatile size_t index=3;return (int)value[index];
#else
    containers();pthread_t threads[8];
    for(unsigned long i=0;i<8;++i)require(pthread_create(&threads[i],0,worker,(void *)(i+1))==0);
    for(unsigned long i=0;i<8;++i) {void *value=0;
        require(pthread_join(threads[i],&value)==0 && value==(void *)(i+1));}
    const char pass[]="native GCC vector/hash/rehash/destructors/alignment/8workers PASS\n";
    require(write(1,pass,sizeof(pass)-1)==(ssize_t)(sizeof(pass)-1));return 100;
#endif
}
