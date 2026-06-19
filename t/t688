\e 0
/ amend self-reference must snapshot the value, not store a live ref.
/ x[0]:(value of x) gives x[0] a finite SNAPSHOT; without the copy it
/ would be x[0]=x -- a cycle that leaks and OOMs when printed.  These hit
/ the kamendi (index) amend paths; AFL never reached them (they need a
/ self-referential .k namespace read, which random fuzzing won't synthesize).
x:(1;2;3)
.[`x;0;{.k.x}]
x
/ deep list index self-ref
y:((0;0);(0;0))
.[`y;(0;0);{.k.y}]
y
/ dict value self-ref via index-amend
d:.()
d.a:1
.[`d;`a;{.k.d}]
d.a
\\
