#include "prx/libc/include/general/AsmMacros.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"
#include "prx/libkernel/System/include/UserContext.hpp"

struct RegisterFrame;

extern "C" {
int* APS5_VABI __error_nid_postfix();
int APS5_VABI Aps5GetcontextFinish_nid_no_patch(UserUcontext* context, const RegisterFrame* frame);
int APS5_VABI setcontext_nid_postfix(const UserUcontext* context);
}

asm(".text\n"
    APS5_ASM_FUNCTION("swapcontext_nid_postfix")
    "    pushfq\n"
    "    push %rbp\n"
    "    mov %rsp, %rbp\n"
    "    sub $0x2a0, %rsp\n"
    "    and $-16, %rsp\n"
    "    mov %rax, 0x00(%rsp)\n"
    "    mov %rbx, 0x08(%rsp)\n"
    "    mov %rcx, 0x10(%rsp)\n"
    "    mov %rdx, 0x18(%rsp)\n"
    "    mov %rsi, 0x20(%rsp)\n"
    "    mov %rdi, 0x28(%rsp)\n"
    "    mov 0(%rbp), %rax\n"
    "    mov %rax, 0x30(%rsp)\n"
    "    lea 24(%rbp), %rax\n"
    "    mov %rax, 0x38(%rsp)\n"
    "    mov %r8, 0x40(%rsp)\n"
    "    mov %r9, 0x48(%rsp)\n"
    "    mov %r10, 0x50(%rsp)\n"
    "    mov %r11, 0x58(%rsp)\n"
    "    mov %r12, 0x60(%rsp)\n"
    "    mov %r13, 0x68(%rsp)\n"
    "    mov %r14, 0x70(%rsp)\n"
    "    mov %r15, 0x78(%rsp)\n"
    "    mov 16(%rbp), %rax\n"
    "    mov %rax, 0x80(%rsp)\n"
    "    mov 8(%rbp), %rax\n"
    "    mov %rax, 0x88(%rsp)\n"
    "    xor %eax, %eax\n"
    "    mov %cs, %ax\n"
    "    mov %rax, 0x90(%rsp)\n"
    "    mov %ss, %ax\n"
    "    mov %rax, 0x98(%rsp)\n"
    "    fxsave64 0xa0(%rsp)\n"
    "    mov %rsi, %rdx\n"
    "    mov %rsp, %rsi\n"
    "    call " APS5_ASM_SYMBOL("Aps5SwapcontextFinish_nid_no_patch") "\n"
    "    mov %rbp, %rsp\n"
    "    pop %rbp\n"
    "    lea 8(%rsp), %rsp\n"
    "    ret\n");

extern "C" int APS5_VABI Aps5SwapcontextFinish_nid_no_patch(UserUcontext* saved, const RegisterFrame* frame, const UserUcontext* next) {
    constexpr int GuestEinval = 22;
    if (saved == nullptr || next == nullptr) {
        *__error_nid_postfix() = GuestEinval;
        return -1;
    }
    if (Aps5GetcontextFinish_nid_no_patch(saved, frame) != 0) return -1;
    return setcontext_nid_postfix(next);
}
