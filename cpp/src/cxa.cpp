/* guard variables */

/* The ABI requires a 64-bit type.  */
__extension__ typedef int __guard __attribute__((mode(__DI__)));

extern "C" int __cxa_guard_acquire (__guard *);
extern "C" void __cxa_guard_release (__guard *);
extern "C" void __cxa_guard_abort (__guard *);

extern "C" int __cxa_guard_acquire (__guard *g) 
{
  return !*(char *)(g);
}

extern "C" void __cxa_guard_release (__guard *g)
{
  *(char *)g = 1;
}

extern "C" void __cxa_guard_abort (__guard *)
{

}

extern "C" void __cxa_pure_virtual()
{
}

extern "C" void *__dso_handle = 0; //Attention! Optimally, you should remove the '= 0' part and define this in your asm script.

extern "C" int __cxa_atexit(void (*f)(void *), void *objptr, void *dso)
{
	return 0;
}

extern "C" void __cxa_finalize(void *f)
{
}
