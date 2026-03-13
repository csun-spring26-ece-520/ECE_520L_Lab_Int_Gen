#include <stdio.h>
#include <stdint.h>
#include "platform.h"
#include "xparameters.h"
#include "xparameters_ps.h"
#include "xil_printf.h"
#include "xil_io.h"
#include "xscugic.h"
#include "xil_exception.h"
#include "xstatus.h"

#define INTR_GEN_BASEADDR   (0x43C00000)
#define INTC_DEVICE_ID      XPAR_SCUGIC_SINGLE_DEVICE_ID
#define INTR_GEN_INTR_ID    XPS_FPGA0_INT_ID

#define FPGA_REV_REG		0x00
#define INT_EN_REG          0x04
#define INT_CLR_REG         0x08
#define INT_STAT_REG      	0x0C
#define GEN_INT_REG         0x10

#define BIT0_MASK           0x00000001

static XScuGic IntcInst;
static volatile uint32_t g_interrupt_count = 0;
static volatile uint32_t g_interrupt_received = 0;

static inline void IntrGen_WriteReg(uint32_t offset, uint32_t value)
{
    Xil_Out32(INTR_GEN_BASEADDR + offset, value);
}

static inline uint32_t IntrGen_ReadReg(uint32_t offset)
{
    return Xil_In32(INTR_GEN_BASEADDR + offset);
}

static void IntrGen_EnableInterrupt(void)
{
    IntrGen_WriteReg(INT_EN_REG, BIT0_MASK);
}

static void IntrGen_DisableInterrupt(void)
{
    IntrGen_WriteReg(INT_EN_REG, 0x00000000);
}

static void IntrGen_ClearInterrupt(void)
{
    IntrGen_WriteReg(INT_CLR_REG, BIT0_MASK);
}

static void IntrGen_GenerateInterrupt(void)
{
    IntrGen_WriteReg(GEN_INT_REG, BIT0_MASK);
}

static void PrintRegisters(void)
{
    xil_printf("REG[0x%02X] FPGA_REV = 0x%08X\r\n", FPGA_REV_REG, (uint32_t)IntrGen_ReadReg(FPGA_REV_REG));

    xil_printf("REG[0x%02X] INT_EN = 0x%08X\r\n", INT_EN_REG, (uint32_t)IntrGen_ReadReg(INT_EN_REG));

    xil_printf("REG[0x%02X] INT_STAT = 0x%08X\r\n", INT_STAT_REG, (uint32_t)IntrGen_ReadReg(INT_STAT_REG));

    xil_printf("REG[0x%02X] GEN_INT = 0x%08X\r\n", GEN_INT_REG, (uint32_t)IntrGen_ReadReg(GEN_INT_REG));
}

static void IntrGen_Isr(void *CallbackRef)
{
    (void)CallbackRef;

    g_interrupt_count++;
    g_interrupt_received = 1;

    xil_printf("ISR: Interrupt received. Count = %d\r\n", g_interrupt_count);

    PrintRegisters();

    IntrGen_ClearInterrupt();

    xil_printf("ISR: Interrupt clear command issued.\r\n");
}

static int SetupInterruptSystem(XScuGic *IntcInstancePtr)
{
    int status;
    XScuGic_Config *IntcConfig;

    IntcConfig = XScuGic_LookupConfig(INTC_DEVICE_ID);

    if (IntcConfig == NULL)
    {
        return XST_FAILURE;
    }

    status = XScuGic_CfgInitialize(IntcInstancePtr, IntcConfig, IntcConfig->CpuBaseAddress);
    if (status != XST_SUCCESS)
    {
        return XST_FAILURE;
    }

    XScuGic_SetPriorityTriggerType(IntcInstancePtr,
                                   INTR_GEN_INTR_ID,
                                   0xA0,
                                   0x3);

    status = XScuGic_Connect(IntcInstancePtr,
                             INTR_GEN_INTR_ID,
                             (Xil_InterruptHandler)IntrGen_Isr,
                             NULL);

    if (status != XST_SUCCESS)
    {
        return XST_FAILURE;
    }

    XScuGic_Enable(IntcInstancePtr, INTR_GEN_INTR_ID);

    Xil_ExceptionInit();

    Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,
                                 (Xil_ExceptionHandler)XScuGic_InterruptHandler,
                                 IntcInstancePtr);

    Xil_ExceptionEnable();

    return XST_SUCCESS;
}

int main(void)
{
    int status;
    volatile uint32_t timeout;

    init_platform();

    xil_printf("Start of Interrupt Generator Test\r\n");

    xil_printf("Base Address: 0x%08lX\r\n", (uint32_t)INTR_GEN_BASEADDR);
    xil_printf("Interrupt ID: %d\r\n", (uint32_t)INTR_GEN_INTR_ID);

    status = SetupInterruptSystem(&IntcInst);
    if (status != XST_SUCCESS)
    {
        xil_printf("ERROR: Interrupt setup failed\r\n");
        cleanup_platform();
        return -1;
    }

    xil_printf("GIC setup complete\r\n");

    PrintRegisters();

    xil_printf("Clearing int_en...\r\n");
    IntrGen_DisableInterrupt();
    xil_printf("[INT_EN_REG] INT_EN = 0x%08X\r\n", INT_EN_REG, (uint32_t)IntrGen_ReadReg(INT_EN_REG));

    xil_printf("Setting int_en...\r\n");
    IntrGen_EnableInterrupt();
    xil_printf("[INT_EN_REG] INT_EN = 0x%08X\r\n", INT_EN_REG, (uint32_t)IntrGen_ReadReg(INT_EN_REG));

    xil_printf("Generating an interrupt...\r\n");
    g_interrupt_received = 0;
    IntrGen_GenerateInterrupt();

    timeout = 100000000U;
    while ((g_interrupt_received == 0) && (timeout > 0))
    {
        timeout--;
    }

    if (g_interrupt_received != 0)
    {
        xil_printf("Interrupt successfully received by Zynq PS\r\n");
    }
    else
    {
        xil_printf("ERROR: Timed out waiting for interrupt\r\n");
    }

    xil_printf("Reading registers after interrupt test...\r\n");
    PrintRegisters();

    xil_printf("Interrupt Count: %d\r\n", (uint32_t)g_interrupt_count);

    xil_printf("End of Interrupt Generator Test\r\n");

    cleanup_platform();
    return 0;
}
