"""Adversarial unit checks for the static weak TLS call-site checker, not a guest test."""
import sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from check_cobalt_tls import audit_instructions,WEAK

def main():
    valid=[' 10: cmpq $0x0,0x0(%rip) # 621cc10 <weak GOT>',
           ' 18: je 20 <skip>',f' 1a: call 60b8570 <{WEAK}@plt>',
           ' 20: ret']
    assert audit_instructions(valid)==[(0x1a,0x10)]
    # Accept a branch that also skips the subsequent emulated TLS work.
    far=list(valid);far[1]=' 18: je 40 <skip>'
    assert audit_instructions(far)==[(0x1a,0x10)]
    compound=[
        ' 22951eb: cmpq $0x0,0x0(%rip) # 621cc10 <weak GOT>',
        ' 22951f3: je 2295219 <null path>',f' 22951f5: call 60b8570 <{WEAK}@plt>',
        ' 22951fa: lea 0x0(%rip),%rdi # 6244cc0 <emutls data>',
        ' 2295201: call 60b7da0 <__emutls_get_address@plt>',
        ' 2295206: cmpl $0x0,(%rax)',' 2295209: je 22952e3 <end>',
        ' 229520f: mov %rax,%r14',f' 2295212: call 60b8570 <{WEAK}@plt>']
    assert audit_instructions(compound)==[(0x22951f5,0x22951eb),(0x2295212,0x22951eb)]
    bad_cases=[[],valid[2:],
               [valid[0],' 18: jne 20 <skip>',*valid[2:]],
               [valid[0],' 18: je 1a <call>',*valid[2:]],
               [' 10: cmpq $0x1,0x0(%rip) # 621cc10 <weak GOT>',*valid[1:]],
               [' 10: cmpq $0x0,0x0(%rip) # 621cc18 <wrong GOT>',*valid[1:]],
               [*valid,' 21: mov 0x0(%rip),%rax # 621cc10 <unexpected reference>']]
    altered=list(compound);altered[1]=' 22951f3: je 229520f <unsafe skip>'
    bad_cases.append(altered)
    altered=list(compound);altered[4]=' 2295201: call 60b7da0 <different_function@plt>'
    bad_cases.append(altered)
    for bad in bad_cases:
        try:audit_instructions(bad)
        except ValueError:pass
        else:raise AssertionError(bad)
    print('PASS: weak TLS static guard checker rejects missing/wrong guard, branch and unexpected references')

if __name__=='__main__':main()
