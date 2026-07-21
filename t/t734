/ algorithm cross-checks: two independent implementations must agree.  The inputs
/ are big random matrices/vectors, so the AGREEMENT is the interesting value.
INF:1000000
mkI:{[m] I:(2#m)#0.0; j:0; while[j<m; I[j;j]:1.0; j:j+1]; I}
/ random weighted digraph as matrix (INF = no edge), diag 0
n:12
W:(1+draw[n*n;30])+INF*draw[n*n;4]>2
W:INF&(2#n)#W
i:0; while[i<n; W[i;i]:0; i:i+1]
/ dijkstra from src (O(n^2) scan)
dij:{[W;src] n:#W; dist:INF*1+!n; dist[src]:0; done:(#W)#0
  do[n; c:INF+1; u:-1
     j:0; while[j<n; if[(~done j)&dist[j]<c; c:dist j; u:j]; j:j+1]
     if[u>-1; done[u]:1; dist:dist&dist[u]+W u]]
  dist}
/ floyd-warshall
fw:{[W] n:#W; D:W; k:0
  while[k<n; i:0; while[i<n; D[i]:D[i]&D[i;k]+D k; i:i+1]; k:k+1]
  D}
ds:INF&dij[W]'!n
DF:INF&fw W
ds~DF                           / all-pairs dijkstra == floyd-warshall (12x12)
/ matrix: inv/mul identity (well-conditioned)
A:(10.0*mkI 6)+0.01*6 6#draw[36;100]
P:mul[A;inv A]
&/&/1e-8>abs P-mkI 6            / A mul inv A is the identity
1e-6>abs 1-(det A)*det inv A    / det inv A is 1%det A
/ LIS length via binl patience vs brute DP
v:draw[40;100]
lisfast:{pile:!0;i:0;while[i<#x;p:pile binl x i;pile:$[p=#pile;pile,x i;@[pile;p;:;x i]];i:i+1];#pile}
lisbrute:{n:#x;d:(#x)#1;i:1;while[i<n;j:0;while[j<i;if[x[j]<x i;d[i]:d[i]|1+d j];j:j+1];i:i+1];|/d}
(lisfast v;lisbrute v)          / the two LIS lengths must agree
/ big sort properties (radix paths) at 200k
bigf:0.001*draw[200000;1000000]
sf:bigf@<bigf
~|/(1_sf)<-1_sf                 / float radix sort comes out sorted
1e-9>abs 1-(+/bigf)%+/sf        / and is a permutation (same sum)
bigj:0j+draw[200000;100000000]
sj:bigj@<bigj
~|/(1_sj)<-1_sj                 / long radix sort comes out sorted
(+/bigj)=+/sj                   / exact sum, so an exact permutation check
/ hash known vectors -- the digests themselves are the golden
md5 "The quick brown fox jumps over the lazy dog"
sha1 "abc"
sha2 ""
/ hex roundtrip
cs:ci 65+draw[50;26]
(bh hb cs)~cs
\\
