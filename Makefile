./build:
	mkdir -p ./build

./build/usb-next.ino.hex: usb-next.ino keymap.h ./build
	arduino-cli compile --fqbn arduino:avr:micro --output-dir ./build

.PHONY: upload
upload: ./build/usb-next.ino.hex
	arduino-cli upload -p /dev/$(shell readlink /dev/arduino) --fqbn arduino:avr:micro --verbose --input-dir ./build
