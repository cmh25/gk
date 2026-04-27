/ 03 custom .m.g wrapping default .
/ records every sync msg in `hist, then delegates to `.
hist:()
.m.g:{hist::hist,,x; . x}
\m i 15555
