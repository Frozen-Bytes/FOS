#ORIGINALLY taken from xv6-x86 source code
# Context switch
#
#   void context_switch(struct Context **old, struct Context *new);
#
# Save the current registers on the stack, creating
# a struct context, and save its address in *old.
# Switch stacks to new and pop previously-saved registers.

.globl context_switch
context_switch:
  # Save old callee-saved registers
  pushl %ebp
  pushl %eax
  pushl %ebx
  pushl %ecx
  pushl %edx
  pushl %esi
  pushl %edi

  # Switch stacks
  movl 32(%esp), %eax
  movl 36(%esp), %edx
  movl %esp, (%eax)
  movl %edx, %esp

  # Load new callee-saved registers
  popl %edi
  popl %esi
  popl %edx
  popl %ecx
  popl %ebx
  popl %eax
  popl %ebp
  ret
