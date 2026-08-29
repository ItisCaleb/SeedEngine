.file	"jump_context.S"
.text
.p2align 4
.global _jump_context

/*****************************************************/

/* kcontext _jump_context(void *new_context, void *data) */
_jump_context:
  /* rcx is first args in Windows ABI */
  /* store context */
  leaq -0x100(%rsp), %rsp
  movq %rbx,        (%rsp)
  movq %rbp,        0x08(%rsp)
  movq %rdi,        0x10(%rsp)
  movq %rsi,        0x18(%rsp)
  movq %r12,        0x20(%rsp)
  movq %r13,        0x28(%rsp)
  movq %r14,        0x30(%rsp)
  movq %r15,        0x38(%rsp)
  leaq _ret(%rip),  %rax
  movq %rax,        0x40(%rsp)
  movq %rdx,        0x48(%rsp)

  /* store old context location */
  movq %rsp, %r8

  movq (%rcx),     %rbx
  movq 0x08(%rcx), %rbp
  movq 0x10(%rcx), %rdi
  movq 0x18(%rcx), %rsi
  movq 0x20(%rcx), %r12
  movq 0x28(%rcx), %r13
  movq 0x30(%rcx), %r14
  movq 0x38(%rcx), %r15
  /* temporary store rip */
  movq 0x40(%rcx), %r9
  /* restore stack */
  leaq 0x100(%rcx), %rsp

  /* old context as arg and return value */
  movq %r8, %rax
  movq %r8, %rcx

  jmp *%r9
_ret:
  ret
