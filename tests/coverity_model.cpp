void ASSERT(bool fVal) {
	if (fVal == false) {
		__coverity_panic__();
	}
}
void ASSERT(void *ptr) {
	if (ptr == 0) {
		__coverity_panic__();
	}
}

void PERSISTANT_ASSERT(bool fVal) {
	if (fVal == false) {
		__coverity_panic__();
	}
}
void PERSISTANT_ASSERT(void *ptr) {
	if (ptr == 0) {
		__coverity_panic__();
	}
}