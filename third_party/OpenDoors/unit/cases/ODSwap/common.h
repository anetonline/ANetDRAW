#define UT_CUSTOM_ASM_INTERRUPT
#define UT_CUSTOM_ASM_DEPENDENCY

#define UTR_AX     0x0001U
#define UTR_CX     0x0002U
#define UTR_DX     0x0004U
#define UTR_BX     0x0008U
#define UTR_BP     0x0010U
#define UTR_SI     0x0020U
#define UTR_DI     0x0040U
#define UTR_DS     0x0080U
#define UTR_ES     0x0100U
#define UTR_FLAGS  0x0200U
#define UTF_CARRY  0x0001U
#define UT_ASM_MAX_INTERRUPT_STEPS 48

typedef struct UTAsmInterruptStep
{
   unsigned short number;
   unsigned short check;
   UTAsmRegisters expected;
   UTAsmRegisters mask;
   unsigned short set;
   UTAsmRegisters output;
   unsigned short copy_cx_to_ax;
} UTAsmInterruptStep;

static UTAsmInterruptStep ut_asm_steps[UT_ASM_MAX_INTERRUPT_STEPS];
static unsigned short ut_asm_step_count;
static unsigned short ut_asm_step_index;
static UTAsmInterruptStep ut_asm_dependency_steps[UT_ASM_MAX_INTERRUPT_STEPS];
static unsigned short ut_asm_dependency_step_count;
static unsigned short ut_asm_dependency_step_index;

static UTAsmInterruptStep *ut_asm_add_step(unsigned short number)
{
   UTAsmInterruptStep empty = {0};
   UTAsmInterruptStep *step;
   UT_ASSERT(ut_asm_step_count < UT_ASM_MAX_INTERRUPT_STEPS);
   if(ut_asm_step_count >= UT_ASM_MAX_INTERRUPT_STEPS)
      return(&ut_asm_steps[UT_ASM_MAX_INTERRUPT_STEPS - 1]);
   step = &ut_asm_steps[ut_asm_step_count++];
   *step = empty;
   step->number = number;
   return(step);
}

static UTAsmInterruptStep *ut_asm_add_dependency_step(unsigned short index)
{
   UTAsmInterruptStep empty = {0};
   UTAsmInterruptStep *step;
   UT_ASSERT(ut_asm_dependency_step_count < UT_ASM_MAX_INTERRUPT_STEPS);
   if(ut_asm_dependency_step_count >= UT_ASM_MAX_INTERRUPT_STEPS)
      return(&ut_asm_dependency_steps[UT_ASM_MAX_INTERRUPT_STEPS - 1]);
   step = &ut_asm_dependency_steps[ut_asm_dependency_step_count++];
   *step = empty;
   step->number = index;
   return(step);
}

static void ut_asm_reset_steps(void)
{
   ut_asm_step_count = 0;
   ut_asm_step_index = 0;
   ut_asm_dependency_step_count = 0;
   ut_asm_dependency_step_index = 0;
}

static void ut_asm_check_register(unsigned short expected,
   unsigned short actual, unsigned short mask)
{
   if(mask == 0)
      mask = 0xffff;
   UT_ASSERT_EQ_UINT(expected & mask, actual & mask);
}

static void ut_asm_check_registers(const UTAsmInterruptStep *step,
   const UTAsmRegisters far *registers)
{
   if(step->check & UTR_AX) ut_asm_check_register(step->expected.ax, registers->ax, step->mask.ax);
   if(step->check & UTR_CX) ut_asm_check_register(step->expected.cx, registers->cx, step->mask.cx);
   if(step->check & UTR_DX) ut_asm_check_register(step->expected.dx, registers->dx, step->mask.dx);
   if(step->check & UTR_BX) ut_asm_check_register(step->expected.bx, registers->bx, step->mask.bx);
   if(step->check & UTR_BP) ut_asm_check_register(step->expected.bp, registers->bp, step->mask.bp);
   if(step->check & UTR_SI) ut_asm_check_register(step->expected.si, registers->si, step->mask.si);
   if(step->check & UTR_DI) ut_asm_check_register(step->expected.di, registers->di, step->mask.di);
   if(step->check & UTR_DS) ut_asm_check_register(step->expected.ds, registers->ds, step->mask.ds);
   if(step->check & UTR_ES) ut_asm_check_register(step->expected.es, registers->es, step->mask.es);
   if(step->check & UTR_FLAGS) ut_asm_check_register(step->expected.flags, registers->flags, step->mask.flags);
}

static void ut_asm_set_registers(const UTAsmInterruptStep *step,
   UTAsmRegisters far *registers)
{
   if(step->set & UTR_AX) registers->ax = step->output.ax;
   if(step->set & UTR_CX) registers->cx = step->output.cx;
   if(step->set & UTR_DX) registers->dx = step->output.dx;
   if(step->set & UTR_BX) registers->bx = step->output.bx;
   if(step->set & UTR_BP) registers->bp = step->output.bp;
   if(step->set & UTR_SI) registers->si = step->output.si;
   if(step->set & UTR_DI) registers->di = step->output.di;
   if(step->set & UTR_DS) registers->ds = step->output.ds;
   if(step->set & UTR_ES) registers->es = step->output.es;
   if(step->set & UTR_FLAGS) registers->flags = step->output.flags;
   if(step->copy_cx_to_ax) registers->ax = registers->cx;
}

void UTASMCALL ut_asm_interrupt(unsigned interrupt_number,
   UTAsmRegisters far *registers)
{
   UTAsmInterruptStep *step;
   if(ut_asm_step_index >= ut_asm_step_count)
   {
      ut_unexpected_mock(1, "software interrupt");
      return;
   }
   step = &ut_asm_steps[ut_asm_step_index++];
   UT_ASSERT_EQ_UINT(step->number, interrupt_number);
   ut_asm_check_registers(step, registers);
   ut_asm_set_registers(step, registers);
}

void UTASMCALL ut_asm_dependency(unsigned dependency_index,
   UTAsmRegisters far *registers)
{
   UTAsmInterruptStep *step;
   if(ut_asm_dependency_step_index >= ut_asm_dependency_step_count)
   {
      ut_unexpected_mock(1, "assembly dependency");
      return;
   }
   step = &ut_asm_dependency_steps[ut_asm_dependency_step_index++];
   UT_ASSERT_EQ_UINT(step->number, dependency_index);
   ut_asm_check_registers(step, registers);
   ut_asm_set_registers(step, registers);
}

static void ut_asm_assert_steps_complete(void)
{
   UT_ASSERT_EQ_UINT(ut_asm_step_count, ut_asm_step_index);
   UT_ASSERT_EQ_UINT(ut_asm_dependency_step_count,
      ut_asm_dependency_step_index);
}
