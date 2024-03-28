#include "portContext.h"

/* Check the risc_v_chip_specific_extensions.h and/or command line
definitions. */

.global risc_v_exception_handler
.global risc_v_interrupt_handler
.global risc_v_msoftware_interrupt_handler
.global risc_v_mtimer_interrupt_handler
.global risc_v_mexternal_interrupt_handler

.weak risc_v_application_exception_handler
.weak risc_v_application_interrupt_handler
/*-----------------------------------------------------------*/

risc_v_application_exception_handler:
    csrr a0, mcause     /* For viewing in the debugger only. */
    csrr a1, mepc       /* For viewing in the debugger only */
    csrr a2, mtval    /* For viewing in the debugger only */
    j .
/*-----------------------------------------------------------*/

risc_v_application_interrupt_handler:
    csrr a0, mcause     /* For viewing in the debugger only. */
    csrr a1, mepc       /* For viewing in the debugger only */
    csrr a2, mtval    /* For viewing in the debugger only */
    j .
/*-----------------------------------------------------------*/

.section .text.risc_v_exception_handler
risc_v_exception_handler:
    portcontextSAVE_EXCEPTION_CONTEXT
    call risc_v_application_exception_handler
    portcontextRESTORE_CONTEXT
/*-----------------------------------------------------------*/

.section .text.risc_v_interrupt_handler
risc_v_interrupt_handler:
    portcontextSAVE_INTERRUPT_CONTEXT
    call risc_v_application_interrupt_handler
    portcontextRESTORE_CONTEXT
/*-----------------------------------------------------------*/

.section .text.risc_v_msoftware_interrupt_handler
risc_v_msoftware_interrupt_handler:
    portcontextSAVE_INTERRUPT_CONTEXT
    call risc_v_application_interrupt_handler
    portcontextRESTORE_CONTEXT
/*-----------------------------------------------------------*/

.section .text.risc_v_mtimer_interrupt_handler
risc_v_mtimer_interrupt_handler:
    portcontextSAVE_INTERRUPT_CONTEXT
    call risc_v_application_interrupt_handler
    portcontextRESTORE_CONTEXT
/*-----------------------------------------------------------*/

.section .text.risc_v_mexternal_interrupt_handler
risc_v_mexternal_interrupt_handler:
    portcontextSAVE_INTERRUPT_CONTEXT
    call risc_v_application_interrupt_handler
    portcontextRESTORE_CONTEXT
/*-----------------------------------------------------------*/
