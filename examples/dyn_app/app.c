int dyn_loader_entry(void) {
	return func1();
}

int func1(void) {
	return func2() + func2();
}


int func2(void) {
	return 1337;
}
