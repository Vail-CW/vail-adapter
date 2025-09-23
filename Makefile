FQBN_qtpy = adafruit:samd:adafruit_qtpy_m0
FQBN_xiao = Seeeduino:samd:seeed_XIAO_m0
UF2_MOUNT = /mnt/chromeos/removable/Arduino
ARDUINO_DIR = /app/Arduino
BUILDER = flatpak run --command ${ARDUINO_DIR}/arduino-builder cc.arduino.arduinoide

default: build/vail-adapter.qtpy.uf2 build/vail-adapter.xiao.uf2

install: install-xiao
install-xiao: build/vail-adapter.xiao.uf2
	./install.sh $< $(UF2_MOUNT) 
install-qtpy: build/vail-adapter.qtpy.uf2
	./install.sh $< /mnt/chromeos/removable/QTPY_BOOT

clean:
	rm -rf build/*

# uf2conv.py is covered by an MIT license.
build/uf2conv.py: build/uf2families.json
	mkdir -p build
	curl -L https://raw.githubusercontent.com/microsoft/uf2/master/utils/$(@F) > $@
	chmod +x $@
build/uf2families.json:
	mkdir -p build
	curl -L https://raw.githubusercontent.com/microsoft/uf2/master/utils/$(@F) > $@

%.xiao.uf2: %.xiao.bin build/uf2conv.py
	build/uf2conv.py -b 0x2000 -c -o $@ $<

%.qtpy.uf2: %.qtpy.bin build/uf2conv.py
	build/uf2conv.py -b 0x2000 -c -o $@ $<

build/%.qtpy.bin: FQBN = adafruit:samd:adafruit_qtpy_m0
build/%.xiao.bin: FQBN = Seeeduino:samd:seeed_XIAO_m0
build/vail-adapter.%.bin: vail-adapter.ino *.cpp *.h
	mkdir -p build/$*
	arduino-builder \
		-build-cache ~/.cache/arduino \
		-build-path build/$* \
		-core-api-version 10813 \
		-fqbn $(FQBN) \
		-hardware ~/.arduino15/packages \
		-tools $(ARDUINO_DIR)/tools-builder \
		-tools ~/.arduino15/packages \
		-hardware $(ARDUINO_DIR)/hardware \
		-hardware ~/.arduino15/packages \
		-built-in-libraries $(ARDUINO_DIR)/libraries \
		-libraries ~/Arduino/libraries \
		-compile \
		$<
	mv build/$*/vail-adapter.ino.bin $@
	
upload: vail-adapter.ino
	arduino --upload --board $(FQBN) $<
