#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>

#include "py/nlr.h"
#include "py/obj.h"
#include "py/compile.h"
#include "py/runtime0.h"
#include "py/runtime.h"
#include "py/stackctrl.h"
#include "py/gc.h"
#include "py/repl.h"
#include "py/pfenv.h"

#include "tinytest.h"
#include "tinytest_macros.h"

void do_str(const char *src);
inline void do_str(const char *src) {
    mp_lexer_t *lex = mp_lexer_new_from_str_len(MP_QSTR__lt_stdin_gt_, src, strlen(src), 0);
    if (lex == NULL) {
        tt_abort_msg("Lexer initialization error");
    }

    nlr_buf_t nlr;
    if (nlr_push(&nlr) == 0) {
        qstr source_name = lex->source_name;
        mp_parse_node_t pn = mp_parse(lex, MP_PARSE_FILE_INPUT);
        mp_obj_t module_fun = mp_compile(pn, source_name, MP_EMIT_OPT_NONE, true);
        mp_call_function_0(module_fun);
        nlr_pop();
    } else {
        mp_obj_print_exception(printf_wrapper, NULL, (mp_obj_t)nlr.ret_val);
        tt_abort_msg("Uncaught exception");
    }
end:
    ;
}

#include "genhdr/tests.h"

int main() {
    const char a[] = {"sim"};
    mp_stack_set_limit(10240);
    void *heap = malloc(256 * 1024);
    gc_init(heap, (char*)heap + 256 * 1024);
    mp_init();
    int r = tinytest_main(1, (const char **) a, groups);
    mp_deinit();
    printf( "status: %i\n", r);
    return r;
}

void gc_collect(void) {
    gc_collect_start();

    // get the registers and the sp
    jmp_buf env;
    setjmp(env);
    volatile mp_uint_t dummy;
    void *sp = (void*)&dummy;

    // trace the stack, including the registers (since they live on the stack in this function)
    gc_collect_root((void**)sp, ((uint32_t)MP_STATE_VM(stack_top) - (uint32_t)sp) / sizeof(uint32_t));

    gc_collect_end();
}

mp_lexer_t *mp_lexer_new_from_file(const char *filename) {
    return NULL;
}

mp_import_stat_t mp_import_stat(const char *path) {
    return MP_IMPORT_STAT_NO_EXIST;
}

mp_obj_t mp_builtin_open(uint n_args, const mp_obj_t *args, mp_map_t *kwargs) {
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_KW(mp_builtin_open_obj, 1, mp_builtin_open);

void nlr_jump_fail(void *val) {
}
