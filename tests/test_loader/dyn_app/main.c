#include <stdio.h>

int dyn_main(void) {
	int result = func1();
	printf("Printing from dynamic app. Will return %d\n", result);
	return result;
}

int func1(void) {
	return func2() + func2();
}

int func2(void) {
	return 21;
}
