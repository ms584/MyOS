"""Test-only MPFR decimal parsing reference; never linked into MyOS."""
import ctypes as C
import random
import struct
def generate(destination):
    library=C.CDLL('libmpfr.so.6')
    class Value(C.Structure):
        _fields_=[('precision',C.c_long),('sign',C.c_int),('exponent',C.c_long),('limbs',C.c_void_p)]
    pointer=C.POINTER(Value)
    library.mpfr_init2.argtypes=[pointer,C.c_long];library.mpfr_init2.restype=None
    library.mpfr_clear.argtypes=[pointer];library.mpfr_clear.restype=None
    library.mpfr_set_str.argtypes=[pointer,C.c_char_p,C.c_int,C.c_int]
    library.mpfr_get_d.argtypes=[pointer,C.c_int];library.mpfr_get_d.restype=C.c_double
    library.mpfr_get_flt.argtypes=[pointer,C.c_int];library.mpfr_get_flt.restype=C.c_float
    value=Value();library.mpfr_init2(C.byref(value),4096)
    texts=['0','-0','0.1','-0.1','1.5','2.2250738585072014e-308','4.9406564584124654e-324',
           '1.7976931348623157e308','1.1754943508222875e-38','1.401298464324817e-45',
           '3.4028234663852886e38','1.00000000000000011102230246251565404236316680908203125',
           '1.000000059604644775390625','1e-9999','-1e-9999','1e9999','-1e9999']
    rng=random.Random(0xc0ba17)
    for _ in range(192):
        texts.append(('-' if rng.getrandbits(1) else '')+str(rng.randrange(1,10**30))+'e'+str(rng.randrange(-355,281)))
    lines=['struct scan_vector {const char *text;uint64_t double_bits;uint32_t float_bits;};',
           'static const struct scan_vector scan_vectors[]={']
    try:
        for text in texts:
            assert library.mpfr_set_str(C.byref(value),text.encode(),10,0)==0
            d=struct.unpack('<Q',struct.pack('<d',library.mpfr_get_d(C.byref(value),0)))[0]
            f=struct.unpack('<I',struct.pack('<f',library.mpfr_get_flt(C.byref(value),0)))[0]
            lines.append('{"'+text+'",UINT64_C(0x%016x),UINT32_C(0x%08x)},'%(d,f))
    finally:library.mpfr_clear(C.byref(value))
    lines.append('};')
    destination.write_text('\n'.join(lines)+'\n')
    return len(texts)
