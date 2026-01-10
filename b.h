#ifndef B_H
#define B_H

#include "k.h"

K builtin(K f, K a, K x);
K draw(K a, K x);
K dotp(K a, K x);
K at_(K a, K x);
K ss(K a, K x);
K sm(K a, K x);
K vs(K a, K x);
K sv(K a, K x);
K atan2_(K a, K x);
K div_(K a, K x);
K and_(K a, K x);
K or_(K a, K x);
K xor_(K a, K x);
K hypot_(K a, K x);
K rot_(K a, K x);
K shift_(K a, K x);
K encrypt_(K a, K x);
K decrypt_(K a, K x);
K setenv_(K a, K x);
K in_(K a, K x);
K dvl_(K a, K x);

K square(K x);
K sqrt_(K x);
K abs_(K x);
K sleep_(K x);
K ic(K x);
K ci(K x);
K do_(K x);
K while_(K x);
K if_(K x);
K cond_(K x);
K dj(K x);
K jd(K x);
K lt(K x);

K log_(K x);
K exp_(K x);
K sqrt_(K x);
K floor_(K x);
K ceil_(K x);
K sin_(K x);
K cos_(K x);
K tan_(K x);
K asin_(K x);
K acos_(K x);
K atan_(K x);
K sinh_(K x);
K cosh_(K x);
K tanh_(K x);
K erf_(K x);
K erfc_(K x);
K gamma_(K x);
K lgamma_(K x);
K rint_(K x);
K trunc_(K x);
K not__(K x);
K kv_(K x);
K vk_(K x);
K val_(K x);
K bd_(K x);
K db_(K x);
K hb_(K x);
K bh_(K x);
K zb_(K x);
K bz_(K x);
K md5_(K x);
K sha1_(K x);
K sha2_(K x);
K getenv_(K x);
K exit_(K x);
K exit__(K x);
K dir_(K a);

#endif /* B_H */
