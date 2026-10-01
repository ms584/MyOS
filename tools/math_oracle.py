"""Generate test-only constants with host MPFR; production never uses this."""
import ctypes as C
import struct
import random


def generate(destination):
    library = C.CDLL('libmpfr.so.6')
    class Value(C.Structure):
        _fields_ = [('precision', C.c_long), ('sign', C.c_int),
                    ('exponent', C.c_long), ('limbs', C.c_void_p)]
    ptr = C.POINTER(Value)
    def function(name, args, result=C.c_int):
        fn = getattr(library, 'mpfr_' + name)
        fn.argtypes = args
        fn.restype = result
        return fn
    init = function('init2', [ptr, C.c_long], None)
    clear = function('clear', [ptr], None)
    set_d = function('set_d', [ptr, C.c_double, C.c_int])
    get_d = function('get_d', [ptr, C.c_int], C.c_double)
    get_f = function('get_flt', [ptr, C.c_int], C.c_float)
    version = function('get_version', [], C.c_char_p)().decode()
    values = [Value() for _ in range(4)]
    for value in values:
        init(C.byref(value), 256)
        assert value.precision == 256 and value.limbs, 'Unsupported MPFR record ABI'
    set_d(C.byref(values[0]), 1.25, 0)
    assert get_d(C.byref(values[0]), 0) == 1.25 and values[0].sign == 1
    definitions = [
        ('ceil', 'ceil', 1, False, 0), ('floor', 'floor', 1, False, 0),
        ('fabs', 'abs', 1, False, 0), ('rint', 'rint', 1, False, 0),
        ('trunc', 'trunc', 1, False, 0), ('sin', 'sin', 1, False, 2),
        ('cos', 'cos', 1, False, 2), ('log10', 'log10', 1, False, 2),
        ('log2', 'log2', 1, False, 2), ('sqrt', 'sqrt', 1, False, 0),
        ('ceilf', 'ceil', 1, True, 0), ('floorf', 'floor', 1, True, 0),
        ('fabsf', 'abs', 1, True, 0), ('sinf', 'sin', 1, True, 2),
        ('cosf', 'cos', 1, True, 2), ('atanf', 'atan', 1, True, 2),
        ('logf', 'log', 1, True, 2), ('log2f', 'log2', 1, True, 2),
        ('expf', 'exp', 1, True, 2), ('exp2f', 'exp2', 1, True, 2),
        ('sqrtf', 'sqrt', 1, True, 0), ('pow', 'pow', 2, False, 2),
        ('powf', 'pow', 2, True, 2), ('atan2f', 'atan2', 2, True, 2),
        ('fma', 'fma', 3, False, 0), ('fmaf', 'fma', 3, True, 0)]
    unary = [0.0, -0.0, 1.0, -1.0, 0.1, -0.1, 0.5, 1.5, -1.5,
             2.0, 10.0, 100.0, -100.0, 1e-30, -1e-30, 1e30, -1e30,
             1e-300, -1e-300, 1e300, -1e300, float('inf'), -float('inf'), float('nan')]
    random_source = random.Random(0xc0ba17)
    for _ in range(64):
        raw = random_source.getrandbits(64)
        if raw >> 52 & 0x7ff == 0x7ff:
            raw ^= 1 << 52
        unary.append(struct.unpack('<d', struct.pack('<Q', raw))[0])
    binary = [(0.0, 0.0), (-0.0, -1.0), (2.0, 3.0), (2.0, -3.0),
              (-2.0, 3.0), (-2.0, 0.5), (0.5, 10.0), (1e-30, 2.0),
              (1e30, 10.0), (float('inf'), 2.0), (1.0, float('nan'))]
    triple = [(1.0, 2.0, 3.0), (1.0, 1.0, -1.0),
              (1.0+2**-27, 1.0-2**-27, -1.0),
              (1.0+2**-12, 1.0-2**-12, -1.0),
              (1e-30, 1e-30, 1e-40), (1e30, 1e30, -1e30),
              (float('inf'), 0.0, 1.0)]
    def bits(value):
        return struct.unpack('<Q', struct.pack('<d', value))[0]
    lines = ['/* Test-only MPFR ' + version + ', 256-bit RNDN constants. */',
             'struct math_case { unsigned operation, is_float, ulps; uint64_t x,y,z,expected; };',
             'static const struct math_case cases[] = {']
    count = 0
    for index, (name, oracle, arity, single, ulps) in enumerate(definitions):
        no_rounding = oracle in ('ceil', 'floor', 'trunc')
        fn = function(oracle, [ptr] * (arity+1) + ([] if no_rounding else [C.c_int]))
        inputs = [(x,) for x in unary] if arity == 1 else binary if arity == 2 else triple
        for row in inputs:
            row = tuple(C.c_float(x).value if single else x for x in row)
            for value, x in zip(values, row):
                set_d(C.byref(value), x, 0)
            fn(C.byref(values[3]), *[C.byref(values[i]) for i in range(arity)],
               *([] if no_rounding else [0]))
            expected = (get_f if single else get_d)(C.byref(values[3]), 0)
            padded = row + (0.0,) * (3-arity)
            numbers = ','.join('UINT64_C(0x%016x)' % bits(x) for x in (*padded, expected))
            lines.append('{%d,%d,%d,%s},' % (index, single, ulps, numbers))
            count += 1
    lines.append('};\nstatic double evaluate(unsigned op,double x,double y,double z) { switch(op) {')
    for index, (name, _, arity, single, _) in enumerate(definitions):
        arguments = ','.join(('(float)' if single else '') + x for x in ('x','y','z')[:arity])
        lines.append('case %d:return %s(%s);' % (index, name, arguments))
    lines.append('default:__builtin_trap(); } }')
    destination.write_text('\n'.join(lines) + '\n')
    for value in values:
        clear(C.byref(value))
    print('Generated test-only MPFR', version, 'cases:', count, flush=True)
