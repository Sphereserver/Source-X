// Avoid false-positive issues by giving an alternative definition of Assert_Fail, used by ASSERT and PERSISTANT_ASSERT.
void Assert_Fail(const char *, const char *, long long)
{
	__coverity_panic__();
}