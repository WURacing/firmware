## MicroPython for Pico Software Guide

The Pico running MicroPython talks to a computer via USB. There's a variety of programs which can do this for you, from very lightweight, purpose-built applications to larger multipurpose software you may already be familiar with. I've only chosen software that works on all major platforms (Windows, Linux, Mac), but there's other great options available for specific platforms as well.

Before anything else, take a look at the official Raspberry Pi Pico's [Getting Started](https://www.raspberrypi.org/documentation/rp2040/getting-started/#getting-started-with-micropython) page for setting up MicroPython.

You can also follow this more comprehensive tutorial that runs from initial setup to running some basic programs, (and stop if you don't want to use Thonny: [Flashing MicroPython on the Pico, then connecting with Thonny](https://magpi.raspberrypi.org/articles/programming-raspberry-pi-pico-with-python-and-micropython)

### There are two vital features we need software for in order to program our Picos:

<dl>
  <dt>
1. Help us develop MicroPython code:
  </dt>
  <dd>

Most Python IDEs will allow for MicroPython development, though some don't specifically support it. If you have one you really love, you can keep using it.

  </dd>
  <dt>
2. Communicate with the Pico:
  </dt>
  <dd>
    
No matter what, you'll need software that can perform serial communication with your Pico. This allows you to upload Python files and access the Pico's on board [REPL](https://en.wikipedia.org/wiki/Read%E2%80%93eval%E2%80%93print_loop) (Python's interactive command loop).
  
  </dd>
</dl>

If you're familiar with the Arduino IDE, then you might be hoping for a MicroPython equivalent; an IDE that lets you develop and upload code directly from the same application. Well good news, many IDEs can also do communication/upload for you, so you just need one program! The most popular IDE by far is Thonny (see below).

### IDEs with built in communication

Applications designed for development of Python code, which also can be used to access REPL on the board and upload Python files. Much bigger than the devoted communication software below, but everything is integrated for you. You do not need other communication software if you use one of these (such as rshell).

| Software | Link | Summary |
| --- | --- | --- |
| Thonny | See line 2 | MicroPython's recommended environment. Code development is convenient and connecting to the Pico is straightforward. |
| VS Code + Pico-Go | [Quick Start](http://pico-go.net/docs/start/quick/) | With the addition of the Pico-Go extension, VS Code is also a very effective IDE for MicroPython. |

### Standalone Communication Software

Gives access to REPL on the board and ability to upload Python files. Development happens in another application, so you get to use any IDE you want (you could literally use notepad if you wanted to for some reason).

| Software | Link | Summary |
| --- | --- | --- |
| rshell | [Tutorial](https://www.twilio.com/blog/programming-raspberry-pi-pico-microcontroller-micropython) | Very slim command line shell. Purpose built for MicroPython |
| ampy | [Tutorial](https://www.digikey.com/en/maker/projects/micropython-basics-load-files-run-code/fb1fcedaf11e4547943abfdd8ad825ce) | Adafruit's MicroPython shell. Similar to rshell, but I haven't tried it. |
| Putty | [Sorta tutorial](https://learn.adafruit.com/micropython-basics-how-to-load-micropython-on-a-board/serial-terminal) | Application mostly known for graphical SSH, but also can be used for UART communication. It's way overkill, but many people are already comfortable with it. |
