"""LTO-aware QEMU symbol lookup must not guess ambiguous private objects."""
from test_system import parse_symbols

def main():
    result=parse_symbols('00001000 B mouse.lto_priv.0\n00002000 b mouse.lto_priv.1\n'
                         '00003000 B x_position.lto_priv.0\n'
                         '00004000 B exact\n00005000 B exact.lto_priv.2\n'
                         '00006000 B bad.lto_priv.word\n                 U unresolved\n')
    assert 'mouse' not in result
    assert result['mouse.lto_priv.0']==0x1000 and result['mouse.lto_priv.1']==0x2000
    assert result['x_position']==result['x_position.lto_priv.0']==0x3000
    assert result['exact']==0x4000 and result['exact.lto_priv.2']==0x5000
    assert 'bad' not in result and 'unresolved' not in result
    assert parse_symbols('')=={}
    print('PASS: exact/unique LTO names; ambiguous/malformed aliases rejected')

if __name__=='__main__':main()
