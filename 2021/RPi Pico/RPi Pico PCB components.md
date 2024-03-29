### Currently looking at designs components with the RPi Pico/RP2040 chip

General redesign help:
 - https://www.reddit.com/r/raspberry_pi/comments/oox4cn/i_designed_a_rp2040_based_raspberry_pi_pico_clone/
 - https://www.raspberrypi.com/documentation/microcontrollers/raspberry-pi-pico.html
 
Not a component, but consider conforming to RPi's RPi Pico or RPi 4 pin layout for hat and/or sock compatibility
 - https://www.hackster.io/news/the-pico-2-pi-adaptor-plate-proves-that-sometimes-it-s-the-simple-things-in-life-that-help-out-7f27e1de6208
 - https://github.com/raspberrypi/hats

RP2040 Chip:
 - https://www.pishop.us/product/raspberry-pi-rp2040/?src=raspberrypi
 ~ $1 (max purchase 2)
 - https://twitter.com/jlcpcb/status/1408343801865728002?s=21
 ~ $1.50 through JLCPCB, neat
 
Power: (regulator necessary for 5V to 3.3V for Pico

Flash: (where the Pico code is stored)

Crystal oscillator: (required for precise timing/communication, i.e. USB, SPI, etc)

IOs:
 - USB Programming/REPL Connector:
   - USB C, through-hole: 
     - https://www.digikey.com/en/products/base-product/cnc-tech/1175/C-ARA1-AK/567424
     ~ $4
     - https://www.digikey.com/en/products/detail/gct/USB4085-GF-A/9859662
     ~ $1
   - USB A, through-hole:
     - https://www.digikey.com/en/products/detail/amphenol-icc-commercial-products/UE27AC54100/1972253
     ~ $0.50
     - https://www.digikey.com/en/products/detail/on-shore-technology-inc/USB-A1HSW6/2677750
     ~ $0.50
  - GPIO: some may still be available, but majority devoted to CAN or ADC

Analog to Digital (ADC):
 - Pretty good, relatively cheap series: ADS7951 (ADS79xx series)
   - 12-Bit, 16-Channel input
   - 1-MSPS sampling
   - SPI serial output
   - https://www.digikey.com/en/products/detail/texas-instruments/ADS7953SBDBTR/1895851
   ~ $10

CAN Bus:
 - The CANPico designs themselves will be helpful here: https://kentindell.github.io/canpico
 - CAN controller: Microchip MCP2518FD CAN controller (used by CANPico)
  - https://www.microchip.com/en-us/product/MCP2518FD
  ~ $2 (out of stock)
 - CAN tranciever: Microchip MCP2562FD CAN transceiver (used by CANPico)
  - https://www.microchip.com/en-us/product/MCP2562FD
  ~ $1 (out of stock)
