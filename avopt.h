#ifndef AVOPT_H
#define AVOPT_H

#include "k.h"

/* dyadic */
K* plus2_avopt(K *a, char *av);
K* minus2_avopt(K *a, char *av);
K* times2_avopt(K *a, char *av);
K* divide2_avopt(K *a, char *av);
K* minand2_avopt(K *a, char *av);
K* maxor2_avopt(K *a, char *av);
K* less2_avopt(K *a, char *av);
K* more2_avopt(K *a, char *av);
K* equal2_avopt(K *a, char *av);
K* power2_avopt(K *a, char *av);
K* modrot2_avopt(K *a, char *av);
K* match2_avopt(K *a, char *av);
K* join2_avopt(K *a, char *av);
K* take2_avopt(K *a, char *av);
K* drop2_avopt(K *a, char *av);
K* form2_avopt(K *a, char *av);
K* find2_avopt(K *a, char *av);
K* at2_avopt(K *a, char *av);
K* dot2_avopt(K *a, char *av);
K* assign2_avopt(K *a, char *av);

K* fourcolon1_avopt(K *a, char *av);
K* fivecolon1_avopt(K *a, char *av);

K* plus2_avopt2(K *a, K *b, char *av);
K* minus2_avopt2(K *a, K *b, char *av);
K* times2_avopt2(K *a, K *b, char *av);
K* divide2_avopt2(K *a, K *b, char *av);
K* minand2_avopt2(K *a, K *b, char *av);
K* maxor2_avopt2(K *a, K *b, char *av);
K* less2_avopt2(K *a, K *b, char *av);
K* more2_avopt2(K *a, K *b, char *av);
K* equal2_avopt2(K *a, K *b, char *av);
K* power2_avopt2(K *a, K *b, char *av);
K* modrot2_avopt2(K *a, K *b, char *av);
K* match2_avopt2(K *a, K *b, char *av);
K* join2_avopt2(K *a, K *b, char *av);
K* take2_avopt2(K *a, K *b, char *av);
K* drop2_avopt2(K *a, K *b, char *av);
K* form2_avopt2(K *a, K *b, char *av);
K* find2_avopt2(K *a, K *b, char *av);
K* at2_avopt2(K *a, K *b, char *av);
K* dot2_avopt2(K *a, K *b, char *av);
K* assign2_avopt2(K *a, K *b, char *av);

K* fourcolon1_avopt2(K *a, K *b, char*av);
K* fivecolon1_avopt2(K *a, K *b, char*av);

/* monadic */
K* flip_avopt(K *a, char *av);
K* negate_avopt(K *a, char *av);
K* first_avopt(K *a, char *av);
K* reciprocal_avopt(K *a, char *av);
K* where_avopt(K *a, char *av);
K* reverse_avopt(K *a, char *av);
K* upgrade_avopt(K *a, char *av);
K* downgrade_avopt(K *a, char *av);
K* group_avopt(K *a, char *av);
K* shape_avopt(K *a, char *av);
K* enumerate_avopt(K *a, char *av);
K* not_avopt(K *a, char *av);
K* enlist_avopt(K *a, char *av);
K* count_avopt(K *a, char *av);
K* flr_avopt(K *a, char *av);
K* format_avopt(K *a, char *av);
K* unique_avopt(K *a, char *av);
K* atom_avopt(K *a, char *av);
K* value_avopt(K *a, char *av);

#endif /* AVOPT_H */
