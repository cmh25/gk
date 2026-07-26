/ closure_any error unwind must not damage a shared value (t778).
/ a is a >maxr-deep verb train reached by name; f defines a local lambda
/ (flags the frame) and returns a, so the return path runs proj_captures,
/ which trips its depth guard and conservatively reports a capture;
/ closure_any then walks the train and hits its own depth guard.  The old
/ consume-on-error unwind freed the train's interior (owned by the global)
/ and nulled a live slot -> later use of a dereferenced 0.  Now the unwind
/ leaves a intact and the call just errors with 'stack.
a:0=
do[10000;a:,a]
f:{x;g:{};a}
e:@[f;0;:]
e 0
e 1
/ a must still be fully usable: applying it recurses past maxr -> a clean
/ trapped 'stack, not a crash (pre-fix this dereferenced the nulled slot).
/ the apply sits inside a lambda so the train walk happens under the trap.
e2:@[{a 5};0;:]
e2 0
e2 1
/ control: same return shape at shallow depth still converts and applies.
b:0=
do[5;b:,b]
h:{x;i:{};b}
r:h 0
r 5
\\
