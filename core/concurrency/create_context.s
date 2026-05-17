.type _create_context, @function
.text
.global _create_context

/****************************************************

/* kcontext _create_context(void* stack, size_t size) */
_create_context:
  /* rcx is first arg in Windows ABI */
  /* we pass bottom of stack here */ 