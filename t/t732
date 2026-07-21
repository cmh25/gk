/ project euler 1-10 (known answers), self-checking: prints FAIL lines only
r:()
/ e1: multiples of 3 or 5 below 1000
r,:233168j~0j++/&|/0=(!1000)!/3 5 / 0j+: & returns longs past the BIGV cap
/ e2: even fibonacci terms below 4e6
fb:1 1;do[32;fb:fb,+/-2#fb];fb:fb@&fb<4000000
r,:4613732~+/fb@&~fb!2
/ e3: largest prime factor of 600851475143
r,:6857j~|/factor 600851475143j
/ e4: largest palindrome product of two 2-digit numbers
p:,/{x*10+!90}'10+!90
r,:9009~|/p@&{x~|x}'$p
/ e5: smallest number divisible by 1..20
r,:232792560~lcm/1+!20
/ e6: sum-square difference n=100
r,:25164150.0~(sqr +/1+!100)-+/sqr 1+!100
/ e7: 1000th prime
ps:&prime !10000
r,:7919j~0j+ps 999
/ e9: pythagorean triplet a+b+c=120, abc
abc:0
a:1
while[a<40; b:a+1; while[b<60; c:120-a+b; if[(c>b)&(sqr[a]+sqr b)=sqr c; abc:a*b*c]; b:b+1]; a:a+1]
r,:60000~abc
/ e10: sum of primes below 1e5
r,:454396537j~+/0j+&prime !100000
$[&/r;"euler-a all ok";{"FAIL slot ",$x}'&~r]
\\
