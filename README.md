# usb-next #

This is an implementation of an adapter to let you run a NeXT keyboard over USB.

A great deal of credit goes to Limor Fried, who [wrote a
tutorial](https://learn.adafruit.com/usb-next-keyboard-with-arduino-micro/code)
that I used heavily in my early versions of this code. Parts of that code are
reused here under the BSD license.

I used this as a way to learn Arduino, and embedded software work, and hardware
reverse engineering. I don't think this code is especially well-structured or
thought out!

## Compiling and flashing

Requires `arduino-cli`. Use `make upload` to upload to an Arduino Nano.

Looks for the arduino at `/dev/arduino`, which assumes that udev has a rule
like this:

```
SUBSYSTEM=="tty", ACTION=="add", ATTRS{idVendor}=="2341", ATTRS{idProduct}=="8037", SYMLINK+="arduino"
```

(that's in my /etc/udev/rules.d/10-local.rules file).
