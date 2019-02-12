#include "mcu/io_map.h"

namespace io_map
{
    const USART_TypeDef* SPI::Peripheral = USART0;
    const USART_TypeDef* UART_0::Peripheral = UART0;
    const USART_TypeDef* UART_1::Peripheral = UART1;

    const std::uint8_t* ProgramFlash::FlashBase = reinterpret_cast<std::uint8_t*>(0x84000000);
    const std::uint8_t* ProgramFlash::ApplicatonBase = reinterpret_cast<std::uint8_t*>(0x00080000);
    
    const TIMER_TypeDef* RAMScrubbing::TimerHW = TIMER0;

    const ACMP_TypeDef* BSP::Latchup::HW = ACMP0;
    const ACMP_TypeDef* MemoryModule<1>::Comparator = ACMP0;
    const ACMP_TypeDef* MemoryModule<2>::Comparator = ACMP1;
}