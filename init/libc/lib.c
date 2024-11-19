
#define __IN_LIBC
#include "defs.h"

int main(int argc, char** argv, char** envp);

static void reloc(char** envp);

int init(int argc, char** argv, char** envp) {
    reloc(envp);
    return main(argc, argv, envp);
}

static void reloc(char** envp) {
    // get aux
    int i;
    for (i = 0; envp[i]; i++);
    __auxv = (void*)&envp[i+1];

    // reloc dynamic
    void *ehdr = NULL;
    Elf_Phdr *phdrs = NULL;
    int phdr_num = 0;
    auxv_t *auxp;
    for (auxp = __auxv; auxp->type != AT_NULL; auxp++) {
        switch (auxp->type) {
            case AT_PAGESZ:
                __pagesize = auxp->a_un.a_val;
                break;
            case AT_MINSIGSTKSZ:
                __minsigstksize = auxp->a_un.a_val;
                break;
            case AT_PHDR:
                phdrs = auxp->a_un.a_ptr;
                break;
            case AT_PHENT:
                if (auxp->a_un.a_val != sizeof(Elf_Phdr)) {
                    abort("phdr entry size mismatch\n");
                }
                ehdr = (void*)phdrs - 64/* sizeof Elf_Phdr */;
                break;
            case AT_PHNUM:
                phdr_num = (int)auxp->a_un.a_val;
                break;
        }
    }
    if (!phdrs || !phdr_num) {
        abort("cannot find phdr, cannot do reloc\n");
    }
    // write(2, "ehdr: ", 6);
    // writehex(2, (intptr_t)ehdr);
    // write(2, "\n", 1);
    for (i = 0; i < phdr_num; i++) {
        Elf_Phdr *ph = &phdrs[i];
        if (ph->p_type != PT_DYNAMIC) {
            continue;
        }
        Elf_Dyn *dyns = (intptr_t)ph->p_vaddr + ehdr;
        if (ph->p_filesz % sizeof(Elf_Dyn)) {
            abort("dynamic section size mismatch\n");
        }

        Elf_Rela *relas = NULL;
        int rela_nums = 0;

        int dyn_nums = ph->p_filesz / sizeof(Elf_Dyn);
        for (int j = 0; j < dyn_nums; j++) {
            Elf_Dyn *dyn = &dyns[j];
            switch (dyn->d_tag) {
                // TODO: REL and RELR, arch specific
                case DT_RELA:
                    relas = dyn->d_un.d_val + ehdr;
                    // write(2, "rela: ", 6);
                    // writehex(2, (intptr_t)relas);
                    // write(2, "\n", 1);
                    break;
                case DT_RELAENT:
                    if (dyn->d_un.d_val != sizeof(Elf_Rela)) {
                        abort("rela entry size mismatch\n");
                    }
                    break;
                case DT_RELASZ:
                    rela_nums = dyn->d_un.d_val / sizeof(Elf_Rela);
                    break;
                default:
                    // write(2 ,"dtag: ", 6);
                    // writehex(2, dyn->d_tag);
                    // write(2, "\n", 1);
                    // do nothing
                    break;
            }
        }

        if (relas && rela_nums) {
            // do rela relocation
            for (int j = 0; j < rela_nums; j++) {
                Elf_Rela *rela = &relas[j];
                // write(2, "rela: ", 6);
                // writehex(2, (intptr_t)rela->r_offset);
                // write(2, ", ", 2);
                // writehex(2, rela->r_info);
                // write(2, ", ", 2);
                // writehex(2, (intptr_t)rela->r_addend);
                // write(2, "\n", 1);
                switch (ELF64_R_TYPE(rela->r_info)) {
#if defined(__aarch64__)
                    case R_AARCH64_RELATIVE:
                        *(intptr_t *)((uint64_t)rela->r_offset + ehdr) = rela->r_addend + (intptr_t)ehdr;
#elif defined(__x86_64__)
                    case R_X86_64_RELATIVE:
                        *(intptr_t *)((uint64_t)rela->r_offset + ehdr) = rela->r_addend + (intptr_t)ehdr;
#elif defined(__riscv)
                    case R_RISCV_RELATIVE:
                        *(intptr_t *)((uint64_t)rela->r_offset + ehdr) = rela->r_addend + (intptr_t)ehdr;
#else
# error not supported
#endif
                        break;
                    default:
                        // write(2, "unknown rela type: ", 19);
                        // writehex(2, ELF64_R_TYPE(rela->r_info));
                        // write(2, "\n", 1);
                        break;
                }
            }
        }
    }
}

// stdlib functions for fucking complier
void *memcpy(void *dest, const void *src, size_t n) {
    for (size_t i = 0; i < n; i++) {
        ((char*)dest)[i] = ((char*)src)[i];
    }
    return dest;
}

void *memset(void *s, int c, size_t n) {
    for (size_t i = 0; i < n; i++) {
        ((char*)s)[i] = c;
    }
    return s;
}

int memcmp(const void *s1, const void *s2, size_t n) {
    for (size_t i = 0; i < n; i++) {
        if (((char*)s1)[i] != ((char*)s2)[i]) {
            return ((char*)s1)[i] - ((char*)s2)[i];
        }
    }
    return 0;
}
