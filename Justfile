# board := "TinyCircuits:samd:tinyscreen"
board := "TinyCircuits:samd:tinyzero"

compile project:
	arduino-cli compile --fqbn {{board}} {{project}}

upload project:
	sudo arduino-cli upload -p /dev/ttyACM0 --fqbn {{board}} {{project}}

install:
	arduino-cli core update-index --additional-urls https://tiny-circuits.com/Downloads/ArduinoBoards/package_tinycircuits_index.json
	arduino-cli core install TinyCircuits:samd --additional-urls https://tiny-circuits.com/Downloads/ArduinoBoards/package_tinycircuits_index.json
	arduino-cli lib install RTCZero
	arduino-cli lib install TinyScreen
