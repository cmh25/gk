\d cuda
f:((-6)_.z.filepath),"cuda.so"
gmul:f 2:("gmul";2)
ginit:f 2:("ginit";1)
exec:f 2:("cu_exec";2)     / exec[f;X] is f'X with a scalar lambda f as the kernel; exec[f;(X;Y)] a 2D grid, one thread per (x;y); (X;Y;Z) 3D; a body V/e (+ * & |) reduces to an atom
src:f 2:("cu_src";2)       / the CUDA source generated for exec[f;x]
stats:f 2:("cu_stats";1)   / compile h2d kernel d2h total (ms) and n of the last exec
info:f 2:("cu_info";1)     / a table of the CUDA devices: name bus arch sm cores clock mem bw l2 shmem threads integrated unified driver nvrtc
