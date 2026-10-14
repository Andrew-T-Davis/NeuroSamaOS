CC := clang
LD := ld.lld
GNU_LD := ld
DISK_DIR := disk
EFI_BOOT_DIR := $(DISK_DIR)/EFI/BOOT

.PHONY: all clean clear run-whpx debug-whpx run-kvm debug-kvm run-tcg debug-tcg run-speed-kvm run-speed-tcg iso

DATE_VERSION := $(shell date +\"%Y.%m.%d\")

CCArg = --target=x86_64-unknown-windows-gnu \
        -nostdlib -mno-red-zone -O3 -Wall -m64 \
        -fno-stack-protector \
        -mno-stack-arg-probe -g -fno-builtin \
        -fno-builtin-all -mllvm --max-store-memcpy=999999 \
        -fno-builtin-memcpy -fno-builtin-memset -ffreestanding -nostdinc \
        -fno-strict-aliasing -std=gnu99 -Wextra \
        -Wshadow -Wconversion -mavx2 -msse -fshort-wchar -DBUILD_DATE='$(DATE_VERSION)'

ASArg = --target=x86_64-unknown-windows-gnu \
        -nostdlib -Wall -m64 -g \
        -mllvm --max-store-memcpy=999999

LDArg = -m i386pep --entry=efi_main \
        --image-base=0x1000 --no-undefined \
        --gc-sections --no-dynamicbase

S_OBJS = $(wildcard *.s)
C_OBJS = $(wildcard *.c)
BIG_SOBJS = $(wildcard *.S)

O_OBJS = $(patsubst %.c,%.o,$(C_OBJS)) \
         $(patsubst %.s,%.o,$(S_OBJS)) \
         $(patsubst %.S,%.o,$(BIG_SOBJS)) \
		 words.o AudioData.o

all: boot.efi

%.o: %.c
	$(CC) $(CCArg) -c $< -o $@

%.o: %.s
	$(CC) $(ASArg) -c $< -o $@

%.o: %.S
	$(CC) $(ASArg) -c $< -o $@

words.o: words.bin
	objcopy -I binary -O pe-x86-64 -B i386 $< $@

words.bin: /usr/share/dict/american-english
	printf 'Neuro-Sama\nNeuroSama\nNeuro\nSama\nTempleOS\nTerry\nA.\nDavis\nTerry A. Davis\nVedal\nVeedal\nUNO\nLOL\nL\nF\nAndy\nAndyFish\nFish\nNebulasOS\nAlpha\nPre-Alpha\nBeta\nCQ CQ CQ\nSOS\nCQD\nFIFA\nWC\nWorld Cup\nOSU\nOSU!\n2000\n2001\n2002\n2005\n2006\n2008\n2009\n1999\n1998\n1997\n2014\n2018\n2003\n2004\n2018\n2019\n2026\n2022\nSans\nUT\nMD\nPython\npy\nC\nc\nC++\ncpp\nGAS\ns\n\t\nTeto\nMiku\nCatGirl\nNeko\nNEKO\nMiku Neko\nCatGirl Miku\ndB\nSin\nDouble\nu64\nu32\nu16\nu8\nf80\nf64\nf32\nNya~\nNya\nNyarch\nVGA\nvga\nVga\nqemu\nQEMU\nQemu\nGay\nFemboy\nFem\ngay\fem\femboy\nnote\nqemu.log\nREADME\nREADME.md\nMIT\nGod\nCore\nCQ\nxHCI\nHPET\nAPIC\nAMP\nSMP\nBSP\nAP\nFC\nPASS\nAC96\nPCI\nAudio Buffers\nDouble\nSin\n0x\n0\n1\n2\n3\n4\n5\n6\n7\n8\n9\nA\nB\nC\nD\nE\nF\n' > $@
	cat $< >> $@
	printf '\n' >> $@

AudioData.bin: Test.wav
	cat $< >> $@

AudioData.o: AudioData.bin
	objcopy -I binary -O pe-x86-64 -B i386 $< $@

boot.efi: $(O_OBJS)
	$(LD) $(LDArg) $^ -o boot.exe
	llvm-objcopy --subsystem=efi-app boot.exe $@
	rm boot.exe

QEMU_BASE = \
        -machine q35,hpet=on \
        -global ICH9-LPC.disable_s3=1 \
        -global ICH9-LPC.disable_s4=1 \
        -drive if=pflash,format=raw,unit=0,file=./OVMF.fd,snapshot=on \
        -m 2G \
        -smp 2,sockets=1,cores=2,threads=1 \
        -device ahci,id=ahci \
        -drive file=fat:rw:disk,format=raw,if=none,id=fatdisk \
        -device ide-hd,bus=ahci.0,drive=fatdisk \
        -device qemu-xhci,id=xhci0 \
        -device usb-kbd,bus=xhci0.0 \
        -device usb-mouse,bus=xhci0.0 \
        -vga none \
        -device virtio-vga \
        -display sdl,gl=on \
        -serial stdio \
        -trace events=./xhci.trace \
        -d guest_errors,unimp

QEMU_BASE-ps2 = \
        -machine q35,hpet=on \
        -global ICH9-LPC.disable_s3=1 \
        -global ICH9-LPC.disable_s4=1 \
        -drive if=pflash,format=raw,unit=0,file=./OVMF.fd,snapshot=on \
        -m 2G \
        -smp 2,sockets=1,cores=2,threads=1 \
        -device ahci,id=ahci \
        -drive file=fat:rw:disk,format=raw,if=none,id=fatdisk \
        -device ide-hd,bus=ahci.0,drive=fatdisk \
        -device qemu-xhci,id=xhci0 \
        -vga none \
        -device virtio-vga \
        -display sdl,gl=on \
        -serial stdio \
        -d guest_errors,unimp

QEMU_AUDIO = \
	-audiodev pa,id=audio_ac97 \
	-device ac97,audiodev=audio_ac97 \

run-kvm: boot.efi 
	mkdir -p $(EFI_BOOT_DIR)
	cp boot.efi $(EFI_BOOT_DIR)/BOOTX64.EFI
	rm -rf $(EFI_BOOT_DIR)/iwlwifi_ucode
	cp -r ./iwlwifi_ucode $(EFI_BOOT_DIR)/iwlwifi_ucode
	qemu-system-x86_64 \
		$(QEMU_BASE) \
		$(QEMU_AUDIO) \
		-accel kvm \
		-cpu Skylake-Client-v4,hle=off,rtm=off,+apic,-la57,model-id="Intel(R) Core(TM) i5-12400F CPU @ 2.50GHz" \
		-d guest_errors,unimp -D qemu.log

run-tcg: boot.efi 
	mkdir -p $(EFI_BOOT_DIR)
	cp boot.efi $(EFI_BOOT_DIR)/BOOTX64.EFI
	rm -rf $(EFI_BOOT_DIR)/iwlwifi_ucode
	cp -r ./iwlwifi_ucode $(EFI_BOOT_DIR)/iwlwifi_ucode
	qemu-system-x86_64 \
		$(QEMU_BASE) \
		$(QEMU_AUDIO) \
		-accel tcg \
		-cpu Skylake-Client-v4,hle=off,rtm=off,+apic,-la57,model-id="Intel(R) Core(TM) i5-12400F CPU @ 2.50GHz" \
		-d int,cpu_reset -D qemu.log

debug-kvm: boot.efi 
	mkdir -p $(EFI_BOOT_DIR)
	cp boot.efi $(EFI_BOOT_DIR)/BOOTX64.EFI
	rm -rf $(EFI_BOOT_DIR)/iwlwifi_ucode
	cp -r ./iwlwifi_ucode $(EFI_BOOT_DIR)/iwlwifi_ucode
	qemu-system-x86_64 \
		$(QEMU_BASE) \
		$(QEMU_AUDIO) \
		-accel kvm \
		-cpu Skylake-Client-v4,hle=off,rtm=off,+apic,-la57,model-id="Intel(R) Core(TM) i5-12400F CPU @ 2.50GHz" \
		-s -S -D qemu.log

debug-tcg: boot.efi 
	mkdir -p $(EFI_BOOT_DIR)
	cp boot.efi $(EFI_BOOT_DIR)/BOOTX64.EFI
	rm -rf $(EFI_BOOT_DIR)/iwlwifi_ucode
	cp -r ./iwlwifi_ucode $(EFI_BOOT_DIR)/iwlwifi_ucode
	qemu-system-x86_64 \
		$(QEMU_BASE) \
		$(QEMU_AUDIO) \
		-accel tcg \
		-cpu Skylake-Client-v4,hle=off,rtm=off,+apic,-la57,model-id="Intel(R) Core(TM) i5-12400F CPU @ 2.50GHz" \
		-s -S -D qemu.log

run-speed-kvm: boot.efi 
	mkdir -p $(EFI_BOOT_DIR)
	cp boot.efi $(EFI_BOOT_DIR)/BOOTX64.EFI
	rm -rf $(EFI_BOOT_DIR)/iwlwifi_ucode
	cp -r ./iwlwifi_ucode $(EFI_BOOT_DIR)/iwlwifi_ucode
	qemu-system-x86_64 \
		$(QEMU_BASE) \
		$(QEMU_AUDIO) \
		-accel kvm \
		-cpu host,+apic \
		-d guest_errors,unimp -D qemu.log

run-speed-tcg: boot.efi 
	mkdir -p $(EFI_BOOT_DIR)
	cp boot.efi $(EFI_BOOT_DIR)/BOOTX64.EFI
	rm -rf $(EFI_BOOT_DIR)/iwlwifi_ucode
	cp -r ./iwlwifi_ucode $(EFI_BOOT_DIR)/iwlwifi_ucode
	qemu-system-x86_64 \
		$(QEMU_BASE) \
		$(QEMU_AUDIO) \
		-accel tcg \
		-cpu host,+apic \
		-d guest_errors,unimp -D qemu.log

run-kvm-ps2: boot.efi 
	mkdir -p $(EFI_BOOT_DIR)
	cp boot.efi $(EFI_BOOT_DIR)/BOOTX64.EFI
	rm -rf $(EFI_BOOT_DIR)/iwlwifi_ucode
	cp -r ./iwlwifi_ucode $(EFI_BOOT_DIR)/iwlwifi_ucode
	qemu-system-x86_64 \
		$(QEMU_BASE-ps2) \
		$(QEMU_AUDIO) \
		-accel kvm \
		-cpu Skylake-Client-v4,hle=off,rtm=off,+apic,-la57,model-id="Intel(R) Core(TM) i5-12400F CPU @ 2.50GHz" \
		-d guest_errors,unimp -D qemu.log

run-tcg-ps2: boot.efi 
	mkdir -p $(EFI_BOOT_DIR)
	cp boot.efi $(EFI_BOOT_DIR)/BOOTX64.EFI
	rm -rf $(EFI_BOOT_DIR)/iwlwifi_ucode
	cp -r ./iwlwifi_ucode $(EFI_BOOT_DIR)/iwlwifi_ucode
	qemu-system-x86_64 \
		$(QEMU_BASE-ps2) \
		$(QEMU_AUDIO) \
		-accel tcg \
		-cpu Skylake-Client-v4,hle=off,rtm=off,+apic,-la57,model-id="Intel(R) Core(TM) i5-12400F CPU @ 2.50GHz" \
		-d int,cpu_reset -D qemu.log

debug-kvm-ps2: boot.efi 
	mkdir -p $(EFI_BOOT_DIR)
	cp boot.efi $(EFI_BOOT_DIR)/BOOTX64.EFI
	rm -rf $(EFI_BOOT_DIR)/iwlwifi_ucode
	cp -r ./iwlwifi_ucode $(EFI_BOOT_DIR)/iwlwifi_ucode
	qemu-system-x86_64 \
		$(QEMU_BASE-ps2) \
		$(QEMU_AUDIO) \
		-accel kvm \
		-cpu Skylake-Client-v4,hle=off,rtm=off,+apic,-la57,model-id="Intel(R) Core(TM) i5-12400F CPU @ 2.50GHz" \
		-s -S -D qemu.log

debug-tcg-ps2: boot.efi 
	mkdir -p $(EFI_BOOT_DIR)
	cp boot.efi $(EFI_BOOT_DIR)/BOOTX64.EFI
	rm -rf $(EFI_BOOT_DIR)/iwlwifi_ucode
	cp -r ./iwlwifi_ucode $(EFI_BOOT_DIR)/iwlwifi_ucode
	qemu-system-x86_64 \
		$(QEMU_BASE-ps2) \
		$(QEMU_AUDIO) \
		-accel tcg \
		-cpu Skylake-Client-v4,hle=off,rtm=off,+apic,-la57,model-id="Intel(R) Core(TM) i5-12400F CPU @ 2.50GHz" \
		-s -S -D qemu.log

run-speed-kvm-ps2: boot.efi 
	mkdir -p $(EFI_BOOT_DIR)
	cp boot.efi $(EFI_BOOT_DIR)/BOOTX64.EFI
	rm -rf $(EFI_BOOT_DIR)/iwlwifi_ucode
	cp -r ./iwlwifi_ucode $(EFI_BOOT_DIR)/iwlwifi_ucode
	qemu-system-x86_64 \
		$(QEMU_BASE-ps2) \
		$(QEMU_AUDIO) \
		-accel kvm \
		-cpu host,+apic \
		-d guest_errors,unimp -D qemu.log

run-speed-tcg-ps2: boot.efi 
	mkdir -p $(EFI_BOOT_DIR)
	cp boot.efi $(EFI_BOOT_DIR)/BOOTX64.EFI
	rm -rf $(EFI_BOOT_DIR)/iwlwifi_ucode
	cp -r ./iwlwifi_ucode $(EFI_BOOT_DIR)/iwlwifi_ucode
	qemu-system-x86_64 \
		$(QEMU_BASE-ps2) \
		$(QEMU_AUDIO) \
		-accel tcg \
		-cpu host,+apic \
		-d guest_errors,unimp -D qemu.log

iso: boot.efi
	mkdir -p $(EFI_BOOT_DIR)
	cp boot.efi $(EFI_BOOT_DIR)/BOOTX64.EFI
	rm -rf $(EFI_BOOT_DIR)/iwlwifi_ucode
	cp -r ./iwlwifi_ucode $(EFI_BOOT_DIR)/iwlwifi_ucode
	rm -f disk/NvVars $(EFI_BOOT_DIR)/NvVars NeuroSamaOS.iso boot.img
	@SIZE_KB=$$(($$(du -sk disk | cut -f1) + 8000)); \
	dd if=/dev/zero of=boot.img bs=1K count=$$SIZE_KB conv=sparse,fsync
	mkfs.vfat boot.img
	mcopy -i boot.img -s disk/* ::/
	xorriso -as mkisofs \
		-o NeuroSamaOS.iso \
		-isohybrid-gpt-basdat \
		-e boot.img \
		-no-emul-boot \
		-J -R -V "NeuroSamaOS" \
		-graft-points \
		/boot.img=boot.img
		
	rm -f boot.img
	@echo "ISO generated successfully"

clean:
	sudo rm -fr *.o boot.exe boot.efi boot.dll qemu.log NeuroSamaOS.iso  PciNet.log *.bin *.elf disk

clear:
	sudo rm -fr *.o boot.exe boot.efi boot.dll qemu.log NeuroSamaOS.iso  PciNet.log *.bin *.elf disk
