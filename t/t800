/ a gap count/condition in do/while/if reduces to the internal 0x0; it must
/ surface as a type error, never leak into an argument frame (afl 2026-08-12:
/ inv do[;&] put a raw 0 in the frame dict, then bd{} crashed serializing it)
@[{do[;&]};0;:]
@[{while[;&]};0;:]
@[{if[;&]};0;:]
@[{inv do[;&]};0;:]
/ the count/condition slots still take real values
nul~do[2;3]
i:0
nul~while[i<3;i:i+1]
i
nul~if[1;i:9]
i
\\
