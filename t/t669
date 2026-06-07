\e 0
/ i64/f32 text columns: LF and CRLF (win) must agree, incl sentinels + negatives
("IJE";4 14 8)0:"eje.txt"
("IJE";4 14 8)0:"eje.dos"
/ full binary record: i32 i64 f32 f64 in one schema, and with a skipped column
("ijef";4 8 4 8)1:"ijef.bin"
(" jef";4 8 4 8)1:"ijef.bin"
/ error cases, trapped to (1;errsym) on stdout
.[{("XY";4 5)0:x};,"eje.txt";:]      / bad schema char
.[{("IJ";4)0:x};,"eje.txt";:]        / width not a vector
.[{("IJ";4 5 6)0:x};,"eje.txt";:]    / #types != #widths
.[{("IJE";4 14 9)0:x};,"eje.txt";:]  / row length mismatch
.[{("ij";4 4)1:x};,"ijef.bin";:]     / binary numeric width wrong
.[{("ij";4 8)1:x};,("eje.txt";0;13);:] / binary size not a multiple of record
.[{"z"1:x};,"eje.txt";:]             / bad single-letter type
.[{"i"1:x};,("eje.txt";0;5);:]       / single-letter size not a multiple
\\
