/ A pending outer postfix is not a dyadic left seed for the inner application.
/ In every case below, the unparenthesized f'g'x form must agree with the
/ explicitly grouped f'(g'x) form.  The original report used a 0xc5 verb
/ composition; the same stack mistake also affected the other callable forms.

f:{x}
h:{x,y}
gc:,$'
gb:sqr
gbd:mul
gdraw:draw
gprim:|
gbase:div[;2]
gconv:{x div 2}/
go:+/
gi:sqr'
gm:mul/

/ original reproducer: composed verb followed by each
{x}'(0$'$')'3_!10

/ 0xc5 composition, with a plain or adverbed inner application
(f'gc'1 2)~f'(gc'1 2)
(f'gc 1 2)~f'(gc 1 2)
(f''gc'1 2)~f''(gc'1 2)
(h/gc'1 2)~h/(gc'1 2)
(h\gc'1 2)~h\(gc'1 2)

/ lambdas/projections, convergence, and primitive values
(f'h 1)~f'(h 1)
(f'gconv 8)~f'(gconv 8)
(f'gprim'1 2)~f'(gprim'1 2)
(f'gbase/8)~f'(gbase/8)

/ inline and named builtin values
(f'sqrt/16.0)~f'(sqrt/16.0)
(f'mul/1 2 3)~f'(mul/1 2 3)
(f'draw 3)~f'(draw 3)
(f'gb'1 2)~f'(gb'1 2)
(f'gbd/1 2 3)~f'(gbd/1 2 3)
(f'gdraw 3)~f'(gdraw 3)

/ derived primitive/builtin values
(f'go 1 2 3)~f'(go 1 2 3)
(f'go'(1 2;3 4))~f'(go'(1 2;3 4))
(f'gi 1 2)~f'(gi 1 2)
(f'gm 1 2 3)~f'(gm 1 2 3)

/ predefined dyad, both partial and adverbed
(f'lin 1 2 3)~f'(lin 1 2 3)
(f'lin'(1 2;2 3))~f'(lin'(1 2;2 3))
\\
