# LaserText
Very simple text laser projector.
Article in English about it: https://habr.com/ru/post/438618/  
Article in Russian about it: https://habr.com/ru/post/407591/

Main parts of it are: polygon mirror module, laser, BLDC motor from DVD with glued mirror, MCU board, photosensor, bread board, ULN2003.

MCU board is Blue Pill (STM32F103C8T6).

Laser - any suitable diode laser (from a laser pointer (weak) or from DVD - powerful, but dangerous for eyes).

Polygon mirror module (from a laser printer) - I think that any that you can run is suitable.
  
See "FW_structure.png" and "TIM2_sequence.png" - description about how FW is working.  
  
Currently horizontal line length -  256 pixels. Vertical number of lines - 24.  
