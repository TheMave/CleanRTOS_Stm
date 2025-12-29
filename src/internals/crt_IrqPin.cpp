// by Marius Versteegen, 2025

#include "crt_IrqPin.h"

namespace crt {

IrqPin::IsrCtxFn IrqPin::s_ctxFn[16]      = { nullptr };
void*            IrqPin::s_ctx[16]        = { nullptr };
IrqPin::IsrFn    IrqPin::s_legacyFn[16]   = { nullptr };
IrqPin*          IrqPin::s_ownerObj[16]   = { nullptr };
GPIO_TypeDef*    IrqPin::s_ownerPort[16]  = { nullptr };

static inline bool isPowerOfTwo(uint16_t v) { return v && ((v & (v - 1)) == 0); }

IrqPin::IrqPin(GPIO_TypeDef* port,
               uint16_t      pinMask,
               void*         ctx,
               IsrCtxFn      isr,
               IrqTrigger    trigger,
               uint32_t      pull,
               uint32_t      nvicPreemptPrio,
               uint32_t      nvicSubPrio,
               bool          autoDisableOnFire)
: _port(port),
  _pinMask(pinMask),
  _line(pinMaskToLine(pinMask)),
  _irqn(lineToIrq(_line)),
  _enabled(false),
  _autoDisableOnFire(autoDisableOnFire)
{
    if (!isPowerOfTwo(_pinMask) || _line > 15 || isr == nullptr) {
        CRT_ASSERT(false && "IrqPin: invalid pinMask/line or null ISR");
        return;
    }

    claimLineOrAssert();

    s_ctxFn[_line]    = isr;
    s_ctx[_line]      = ctx;
    s_legacyFn[_line] = nullptr;
    s_ownerObj[_line] = this;

    initCommon(trigger, pull, nvicPreemptPrio, nvicSubPrio);
}

IrqPin::IrqPin(GPIO_TypeDef* port,
               uint16_t      pinMask,
               IsrFn         isr,
               IrqTrigger    trigger,
               uint32_t      pull,
               uint32_t      nvicPreemptPrio,
               uint32_t      nvicSubPrio,
               bool          autoDisableOnFire)
: _port(port),
  _pinMask(pinMask),
  _line(pinMaskToLine(pinMask)),
  _irqn(lineToIrq(_line)),
  _enabled(false),
  _autoDisableOnFire(autoDisableOnFire)
{
    if (!isPowerOfTwo(_pinMask) || _line > 15 || isr == nullptr) {
        CRT_ASSERT(false && "IrqPin: invalid pinMask/line or null ISR");
        return;
    }

    claimLineOrAssert();

    s_ctxFn[_line]    = nullptr;
    s_ctx[_line]      = nullptr;
    s_legacyFn[_line] = isr;
    s_ownerObj[_line] = this;

    initCommon(trigger, pull, nvicPreemptPrio, nvicSubPrio);
}

void IrqPin::claimLineOrAssert()
{
    CRT_ASSERT(_line < 16);

    // Only allow claiming an unused line.
    CRT_ASSERT(s_ownerPort[_line] == nullptr && "IrqPin: EXTI line already claimed by another pin");
    CRT_ASSERT(s_ctxFn[_line] == nullptr && s_legacyFn[_line] == nullptr && "IrqPin: EXTI callback already registered");
    CRT_ASSERT(s_ownerObj[_line] == nullptr && "IrqPin: EXTI owner already registered");

    s_ownerPort[_line] = _port;
}

void IrqPin::initCommon(IrqTrigger trigger, uint32_t pull, uint32_t nvicPreemptPrio, uint32_t nvicSubPrio)
{
    enableGpioClock();
    enableSyscfgClock();

    GPIO_InitTypeDef cfg = {0};
    cfg.Pin   = _pinMask;
    cfg.Mode  = triggerToHalMode(trigger);
    cfg.Pull  = pull;
    cfg.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(_port, &cfg);

    HAL_NVIC_SetPriority(_irqn, nvicPreemptPrio, nvicSubPrio);
    HAL_NVIC_EnableIRQ(_irqn);

    // Default: disabled after init
    clearPending();
    disable();
}

void IrqPin::enable() {
    clearPending();
    extiEnable(_pinMask);
    _enabled = true;
}

void IrqPin::disable() {
    extiDisable(_pinMask);
    _enabled = false;
}

void IrqPin::clearPending() {
    extiClearPending(_pinMask);
}

void IrqPin::dispatch(uint16_t pinMask)
{
    uint8_t line = pinMaskToLine(pinMask);
    if (line >= 16) return;

    // Optional: auto-disable immediately on fire to avoid interrupt storms.
    IrqPin* owner = s_ownerObj[line];
    if (owner && owner->_autoDisableOnFire) {
        // Mask first, then clear pending to swallow any bounce edges that arrived during ISR.
        owner->disable();
        owner->clearPending();
    }

    // Prefer ctx callback if present
    IsrCtxFn ctxFn = s_ctxFn[line];
    if (ctxFn) {
        ctxFn(s_ctx[line], pinMask);
        return;
    }

    // Otherwise legacy
    IsrFn legacy = s_legacyFn[line];
    if (legacy) {
        legacy(pinMask);
        return;
    }
}

uint32_t IrqPin::triggerToHalMode(IrqTrigger trigger) {
    switch (trigger) {
        case IrqTrigger::ON_FLANK_UP:   return GPIO_MODE_IT_RISING;
        case IrqTrigger::ON_FLANK_DOWN: return GPIO_MODE_IT_FALLING;
        case IrqTrigger::ON_FLANK_BOTH: return GPIO_MODE_IT_RISING_FALLING;
        default:                        return GPIO_MODE_IT_FALLING;
    }
}

uint8_t IrqPin::pinMaskToLine(uint16_t pinMask) {
    if (!isPowerOfTwo(pinMask)) return 0xFF;
#if defined(__GNUC__)
    return static_cast<uint8_t>(__builtin_ctz(static_cast<unsigned int>(pinMask)));
#else
    uint8_t i = 0;
    while ((pinMask >> i) != 1) { i++; if (i > 15) return 0xFF; }
    return i;
#endif
}

IRQn_Type IrqPin::lineToIrq(uint8_t line)
{
#if defined(STM32WLE5xx) || defined(STM32WL55xx) || defined(STM32WL54xx) || defined(STM32WL5Mxx) || defined(STM32WLxx)
    switch (line) {
        case 0:  return EXTI0_IRQn;
        case 1:  return EXTI1_IRQn;
        case 2:  return EXTI2_IRQn;
        case 3:  return EXTI3_IRQn;
        case 4:  return EXTI4_IRQn;
        default:
            if (line <= 9)  return EXTI9_5_IRQn;
            else            return EXTI15_10_IRQn;
    }
#elif defined(STM32F0xx) || defined(STM32L0xx) || defined(STM32G0xx)
    if (line <= 1) return EXTI0_1_IRQn;
    if (line <= 3) return EXTI2_3_IRQn;
    return EXTI4_15_IRQn;
#else
    switch (line) {
        case 0:  return EXTI0_IRQn;
        case 1:  return EXTI1_IRQn;
        case 2:  return EXTI2_IRQn;
        case 3:  return EXTI3_IRQn;
        case 4:  return EXTI4_IRQn;
        default:
            if (line <= 9)  return EXTI9_5_IRQn;
            else            return EXTI15_10_IRQn;
    }
#endif
}

void IrqPin::enableGpioClock() {
    if (_port == GPIOA) __HAL_RCC_GPIOA_CLK_ENABLE();
    else if (_port == GPIOB) __HAL_RCC_GPIOB_CLK_ENABLE();
    else if (_port == GPIOC) __HAL_RCC_GPIOC_CLK_ENABLE();
#ifdef GPIOD
    else if (_port == GPIOD) __HAL_RCC_GPIOD_CLK_ENABLE();
#endif
#ifdef GPIOE
    else if (_port == GPIOE) __HAL_RCC_GPIOE_CLK_ENABLE();
#endif
#ifdef GPIOF
    else if (_port == GPIOF) __HAL_RCC_GPIOF_CLK_ENABLE();
#endif
#ifdef GPIOG
    else if (_port == GPIOG) __HAL_RCC_GPIOG_CLK_ENABLE();
#endif
#ifdef GPIOH
    else if (_port == GPIOH) __HAL_RCC_GPIOH_CLK_ENABLE();
#endif
}

void IrqPin::enableSyscfgClock() {
#if defined(__HAL_RCC_SYSCFG_CLK_ENABLE)
    __HAL_RCC_SYSCFG_CLK_ENABLE();
#elif defined(__HAL_RCC_AFIO_CLK_ENABLE)
    __HAL_RCC_AFIO_CLK_ENABLE();
#endif
}

// --- Low-level EXTI masking / pending handling -----------------------------

void IrqPin::extiEnable(uint16_t pinMask) {
#if defined(EXTI_IMR1_IM0)
    EXTI->IMR1 |= static_cast<uint32_t>(pinMask);
#elif defined(EXTI_IMR_IM0)
    EXTI->IMR  |= static_cast<uint32_t>(pinMask);
#elif defined(__HAL_GPIO_EXTI_ENABLE_IT)
    __HAL_GPIO_EXTI_ENABLE_IT(pinMask);
#else
    (void)pinMask;
#endif
}

void IrqPin::extiDisable(uint16_t pinMask) {
#if defined(EXTI_IMR1_IM0)
    EXTI->IMR1 &= ~static_cast<uint32_t>(pinMask);
#elif defined(EXTI_IMR_IM0)
    EXTI->IMR  &= ~static_cast<uint32_t>(pinMask);
#elif defined(__HAL_GPIO_EXTI_DISABLE_IT)
    __HAL_GPIO_EXTI_DISABLE_IT(pinMask);
#else
    (void)pinMask;
#endif
}

void IrqPin::extiClearPending(uint16_t pinMask) {
#if defined(EXTI_PR1_PR0)
    EXTI->PR1 = static_cast<uint32_t>(pinMask);
#elif defined(EXTI_PR_PR0)
    EXTI->PR  = static_cast<uint32_t>(pinMask);
#elif defined(__HAL_GPIO_EXTI_CLEAR_IT)
    __HAL_GPIO_EXTI_CLEAR_IT(pinMask);
#else
    (void)pinMask;
#endif
}

} // namespace crt

// ---- Integration hook ------------------------------------------------------
// Call this from your existing HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin).
extern "C" void crt_IrqPin_onHalGpioExti(uint16_t GPIO_Pin)
{
    crt::IrqPin::dispatch(GPIO_Pin);
}

#if defined(CRT_IRQPIN_DEFINE_HAL_GPIO_EXTI_CALLBACK)
extern "C" void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    crt_IrqPin_onHalGpioExti(GPIO_Pin);
}
#endif
