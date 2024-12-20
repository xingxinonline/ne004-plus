#include "ne004xx.h"
#include "ne004xx_gpio.h"

int set_gpio_function(emGPIO port,uint8_t pin,emGPIOFUNC func)
{
    uint32_t temp = port * 32 + pin;
    uint32_t shift,index;
    temp <<= 1;
    index = temp / 32;
    shift = temp % 32;
    temp = IO_Matrtix_CFG(index);
    temp &= ~(0x3 << shift);
    temp |= func << shift;
    IO_Matrtix_CFG(index) = temp;
    return 0;
}

int set_gpio_mode(emGPIO port,uint8_t pin,emGPIOMODE mode)
{
    uint32_t temp = port * 32 + pin;
    uint32_t shift,index;
    temp <<= 1;
    index = temp / 32;
    shift = temp % 32;
    temp = IO_MUX_CFG_CFG(index);
    temp &= ~(0x3 << shift);
    temp |= mode << shift;
    IO_MUX_CFG_CFG(index) = temp;
    return 0;
}

/**
 * The function returns the GPIO port number based on the input parameter.
 * 
 * @param port The parameter "port" is of type "emGPIO", which is likely an enumeration type that
 * represents different GPIO ports. The function takes this parameter as input and returns the
 * corresponding GPIO port number.
 * 
 * @return the GPIO port number corresponding to the input parameter `port`. The GPIO port number is
 * calculated by shifting the value of `gpio_config_reg2` by `port * 5` bits to the right, then
 * performing a bitwise AND operation with `0x1F` (which is a binary number with 5 bits set to 1) and
 * adding 1 to the result
 */
int get_gpio_port_numbers(emGPIO port)
{
    return (((gpio_config_reg2 >> (port * 5)) & 0x1F) + 1);
}

/**
 * The function sets the value of a specific pin in a GPIO port.
 * 
 * @param port The GPIO port to be configured. It is of type emGPIO, which is likely an enumeration
 * that defines the available GPIO ports on the system.
 * @param pin The pin number on the GPIO port to set the value for.
 * @param value The value to be set on the specified GPIO pin. It can be either 0 or 1.
 * 
 * @return an integer value 'res', but it is not being assigned any value or modified within the
 * function. Therefore, it is likely that the return statement is not serving any purpose and can be
 * removed.
 */
int set_gpio_data(emGPIO port,uint8_t pin,uint32_t value)
{
    int res = 0;
    uint32_t temp;
    uint32_t shift = (1 << pin);

    if(pin == (uint8_t)0xFFU){
        temp = value;
    }
    else{
        temp = gpio_swport_dr(port);

        temp &= ~shift;
        if(value) temp |= shift;
    }

    gpio_swport_dr(port) = temp;
    return res;
}


/**
 * The function returns the data of a specified GPIO port.
 * 
 * @param port The parameter "port" is of type "emGPIO", which is likely an enumeration that represents
 * a specific GPIO port on a microcontroller or embedded system. The function "gpio_ext_port" likely
 * reads the data from the specified GPIO port and returns it as a 32-bit unsigned integer (uint32_t
 * 
 * @return The function `get_gpio_data` is returning a 32-bit unsigned integer value that represents the
 * data of the specified GPIO port. The data could be the current state of the GPIO pins, which could
 * be either high or low.
 */
uint32_t get_gpio_data(emGPIO port)
{
    return gpio_ext_port(port);
}

/**
 * The function sets the direction of a GPIO pin as input or output.
 * 
 * @param port The GPIO port to configure. It is of type emGPIO, which is likely an enumeration that
 * defines the available GPIO ports on the system.
 * @param pin The pin number to set the direction for.
 * @param isoutput A uint32_t variable that specifies whether the GPIO pin should be configured as an
 * output (non-zero value) or an input (zero value).
 * 
 * @return an integer value 'res', but it is not being assigned any value or modified within the
 * function. Therefore, it is likely that the function is returning a default value of 0 indicating
 * successful execution.
 */
int set_gpio_direction(emGPIO port,uint8_t pin,uint32_t isoutput)
{
    int res = 0;
    uint32_t temp;
    uint32_t shift = (1 << pin);

    if(pin == (uint8_t)0xFFU){
        temp = isoutput;
    }
    else{
        temp = gpio_swport_ddr(port);

        temp &= ~shift;
        if(isoutput) temp |= shift;
    }

    gpio_swport_ddr(port) = temp;
    return res;
}



/**
 * The function sets the software mode of a GPIO pin.
 * 
 * @param port The GPIO port number, which is an enumerated value of type emGPIO.
 * @param pin The pin number of the GPIO port that needs to be configured.
 * @param issoft issoft is a boolean variable that indicates whether the GPIO pin should be set to
 * software mode or not. If issoft is true, the pin will be set to software mode, otherwise it will be
 * set to hardware mode.
 * 
 * @return an integer value 'res', but it is not being used or modified within the function. Therefore,
 * the value being returned is not relevant to the functionality of the function.
 */
int SetGPIOSoftwareMode(emGPIO port,uint8_t pin,char issoft)
{
    int res = 0;
    uint32_t temp;

    temp = gpio_swport_ctl(port);
#ifdef CONFIG_GPIO_BIT_CTRL
    uint32_t shift = (1 << pin);
    temp &= ~shift;
    if(!issoft) temp |= shift;
#else
    temp = (issoft)?0:1;
    (void)pin;
#endif
    gpio_swport_ctl(port) = temp;
    return res;
}

