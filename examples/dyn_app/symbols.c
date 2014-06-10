#include "symbols.h"
extern int dyn_entry();
extern int func1();
const struct symbols symbols[3] = {
{"dyn_entry", (char *)dyn_entry},
{"func1", (char *)func1},
{(void *)0, 0} };
