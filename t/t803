/ juxtaposition composes verb-class values; application treats function
/ values as nouns, always.  Parentheses close a value, so a grouped train on
/ the right of a juxtaposition is the value the group contains: {x}(<2+)
/ means what c:(<2+); {x} c means.  Bare spellings keep the K3 train
/ reading: {x}<2+ binds {x} as <'s left argument (type error on apply).
c:(<2+)
q:{x}(<2+)
q 1 2 3
q~{x} c
q~({x})(<2+)
lam:{x}
q~lam(<2+)
b:{x}<2+
@[b;1 2 3;:]

/ display parenthesizes nested composition members, so values round-trip
5:q
q~. 5:q
q8:{x}({x}(<2+))
q8 1 2 3
q8~. 5:q8

/ a noun left of a closed value indexes/applies, same as the named spelling;
/ bare 1<2+ keeps the fixed-dyad train reading
@[{q:1(<2+)};0;:]
@[{1 c};0;:]
bt:1<2+
bt 3 1 2

/ verb heads unchanged: composition and display round-trip
vh:-(2+)
vh 3 1 2
vh~. 5:vh
t1:<2+
t1~. 5:t1

/ application contexts: function values pass through as data
v:(1;2.0;2+;`a)
{x}'v
{x}[2+]
{x}@(2+)
\\
