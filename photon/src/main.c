#include <stdint.h>

/*
 * STM32F031C6Tx bare-metal PA14 toggle test.
 *
 * Hardware:
 *   HSE crystal = 8 MHz
 *
 * Firmware:
 *   HSE 8 MHz -> PLL x6 -> SYSCLK 48 MHz
 *   PA14 configured as push-pull output
 *   Infinite fast toggle:
 *      PA14 = 1
 *      PA14 = 0
 */

/* Peripheral base addresses */
#define FLASH_BASE      0x40022000UL
#define RCC_BASE        0x40021000UL
#define GPIOA_BASE      0x48000000UL

#define REG32(addr)     (*(volatile uint32_t *)(addr))

/* FLASH registers */
#define FLASH_ACR       REG32(FLASH_BASE + 0x00UL)

/* RCC registers */
#define RCC_CR          REG32(RCC_BASE + 0x00UL)
#define RCC_CFGR        REG32(RCC_BASE + 0x04UL)
#define RCC_AHBENR      REG32(RCC_BASE + 0x14UL)
#define RCC_CFGR2       REG32(RCC_BASE + 0x2CUL)

/* GPIOA registers */
#define GPIOA_MODER     REG32(GPIOA_BASE + 0x00UL)
#define GPIOA_OTYPER    REG32(GPIOA_BASE + 0x04UL)
#define GPIOA_OSPEEDR   REG32(GPIOA_BASE + 0x08UL)
#define GPIOA_PUPDR     REG32(GPIOA_BASE + 0x0CUL)
#define GPIOA_BSRR      REG32(GPIOA_BASE + 0x18UL)
#define GPIOA_AFRH      REG32(GPIOA_BASE + 0x24UL)

/* FLASH bits */
#define FLASH_ACR_LATENCY       (1UL << 0)
#define FLASH_ACR_PRFTBE        (1UL << 4)

/* RCC_CR bits */
#define RCC_CR_HSEON            (1UL << 16)
#define RCC_CR_HSERDY           (1UL << 17)
#define RCC_CR_PLLON            (1UL << 24)
#define RCC_CR_PLLRDY           (1UL << 25)

/* RCC_CFGR bits */
#define RCC_CFGR_SW_MASK        (3UL << 0)
#define RCC_CFGR_SW_PLL         (2UL << 0)
#define RCC_CFGR_SWS_MASK       (3UL << 2)
#define RCC_CFGR_SWS_PLL        (2UL << 2)

#define RCC_CFGR_HPRE_MASK      (0xFUL << 4)
#define RCC_CFGR_PPRE_MASK      (0x7UL << 8)

#define RCC_CFGR_PLLSRC         (1UL << 16)
#define RCC_CFGR_PLLXTPRE       (1UL << 17)
#define RCC_CFGR_PLLMUL_MASK    (0xFUL << 18)
#define RCC_CFGR_PLLMUL6        (4UL << 18)

/* RCC_AHBENR bits */
#define RCC_AHBENR_GPIOAEN      (1UL << 17)

/* PA14 */
#define PA14_BIT                14UL
#define PA14_MASK               (1UL << PA14_BIT)

static void clock_init_48mhz_from_hse_8mhz(void) {
    /*
     * At 48 MHz, Flash needs 1 wait state and prefetch enabled.
     * Datasheet says 0 wait states up to 24 MHz, 1 wait state above 24 MHz.
     */
    FLASH_ACR = FLASH_ACR_PRFTBE | FLASH_ACR_LATENCY;

    /* Start HSE 8 MHz crystal. */
    RCC_CR |= RCC_CR_HSEON;
    while ((RCC_CR & RCC_CR_HSERDY) == 0UL) {
    }

    /*
     * PREDIV = /1.
     * On STM32F0 this is reset value, but we force it to 0 for clarity.
     */
    RCC_CFGR2 &= ~0xFUL;

    /*
     * AHB prescaler = /1
     * APB prescaler = /1
     * PLL source = HSE/PREDIV
     * PLL multiplier = x6
     *
     * 8 MHz x 6 = 48 MHz
     */
    RCC_CFGR &= ~(RCC_CFGR_HPRE_MASK |
                  RCC_CFGR_PPRE_MASK |
                  RCC_CFGR_PLLSRC |
                  RCC_CFGR_PLLXTPRE |
                  RCC_CFGR_PLLMUL_MASK);

    RCC_CFGR |= RCC_CFGR_PLLSRC | RCC_CFGR_PLLMUL6;

    /* Enable PLL. */
    RCC_CR |= RCC_CR_PLLON;
    while ((RCC_CR & RCC_CR_PLLRDY) == 0UL) {
    }

    /* Select PLL as system clock. */
    RCC_CFGR &= ~RCC_CFGR_SW_MASK;
    RCC_CFGR |= RCC_CFGR_SW_PLL;

    while ((RCC_CFGR & RCC_CFGR_SWS_MASK) != RCC_CFGR_SWS_PLL) {
    }
}

static void pa14_init_output(void) {
    /* Enable GPIOA clock. */
    RCC_AHBENR |= RCC_AHBENR_GPIOAEN;
    (void)RCC_AHBENR;

    /* Force PA14 low before enabling output. */
    GPIOA_BSRR = (PA14_MASK << 16);

    /*
     * PA14 mode = 01: general purpose output.
     * PA14 uses MODER bits 29:28.
     */
    GPIOA_MODER &= ~(3UL << (PA14_BIT * 2UL));
    GPIOA_MODER |=  (1UL << (PA14_BIT * 2UL));

    /* Push-pull output. */
    GPIOA_OTYPER &= ~PA14_MASK;

    /* High speed output. */
    GPIOA_OSPEEDR &= ~(3UL << (PA14_BIT * 2UL));
    GPIOA_OSPEEDR |=  (3UL << (PA14_BIT * 2UL));

    /* No pull-up, no pull-down. */
    GPIOA_PUPDR &= ~(3UL << (PA14_BIT * 2UL));

    /*
     * Clear alternate function for PA14.
     * PA14 is in AFRH, index 6, bits 27:24.
     */
    GPIOA_AFRH &= ~(0xFUL << ((PA14_BIT - 8UL) * 4UL));
}

void main(void) {
    clock_init_48mhz_from_hse_8mhz();
    pa14_init_output();

    for (;;) {
        GPIOA_BSRR = PA14_MASK;          /* PA14 = 1 */
        GPIOA_BSRR = (PA14_MASK << 16);  /* PA14 = 0 */
    }
}
