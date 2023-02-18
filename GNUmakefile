.PHONY: all
all: Blackrock.iso

.PHONY: all-hdd
all-hdd: Blackrock.hdd

.PHONY: run
run: Blackrock.iso
	qemu-system-x86_64 -M q35 -m 2G -cdrom Blackrock.iso -boot d

.PHONY: run-uefi
run-uefi: ovmf-x64 Blackrock.iso
	qemu-system-x86_64 -M q35 -m 2G -bios ovmf-x64/OVMF.fd -cdrom Blackrock.iso -boot d

.PHONY: run-hdd
run-hdd: Blackrock.hdd
	qemu-system-x86_64 -M q35 -m 2G -hda Blackrock.hdd

.PHONY: run-hdd-uefi
run-hdd-uefi: ovmf-x64 Blackrock.hdd
	qemu-system-x86_64 -M q35 -m 2G -bios ovmf-x64/OVMF.fd -hda Blackrock.hdd

ovmf-x64:
	mkdir -p ovmf-x64
	cd ovmf-x64 && curl -o OVMF-X64.zip https://efi.akeo.ie/OVMF/OVMF-X64.zip && 7z x OVMF-X64.zip

limine:
	git clone https://github.com/limine-bootloader/limine.git --branch=v4.x-branch-binary --depth=1
	make -C limine

.PHONY: kernel
kernel:
	$(MAKE) -C kernel

Blackrock.iso: limine kernel
	rm -rf iso_root
	mkdir -p iso_root
	mkdir -p iso_root/fonts
	cp kernel/kernel.elf \
		limine.cfg limine/limine.sys limine/limine-cd.bin limine/limine-cd-efi.bin iso_root/

	cp -rf fonts/* iso_root/fonts/

	xorriso -as mkisofs -b limine-cd.bin \
		-no-emul-boot -boot-load-size 4 -boot-info-table \
		--efi-boot limine-cd-efi.bin \
		-efi-boot-part --efi-boot-image --protective-msdos-label \
		iso_root -o Blackrock.iso
	limine/limine-deploy Blackrock.iso
	rm -rf iso_root

Blackrock.hdd: limine kernel
	rm -f Blackrock.hdd
	dd if=/dev/zero bs=1M count=0 seek=64 of=Blackrock.hdd
	parted -s Blackrock.hdd mklabel gpt
	parted -s Blackrock.hdd mkpart ESP fat32 2048s 100%
	parted -s Blackrock.hdd set 1 esp on
	limine/limine-deploy Blackrock.hdd
	sudo losetup -Pf --show Blackrock.hdd >loopback_dev
	sudo mkfs.fat -F 32 `cat loopback_dev`p1
	mkdir -p img_mount
	sudo mount `cat loopback_dev`p1 img_mount
	sudo mkdir -p img_mount/EFI/BOOT
	sudo cp -v kernel/kernel.elf limine.cfg limine/limine.sys img_mount/
	sudo cp -v limine/BOOTX64.EFI img_mount/EFI/BOOT/
	sync
	sudo umount img_mount
	sudo losetup -d `cat loopback_dev`
	rm -rf loopback_dev img_mount

.PHONY: clean
clean:
	rm -rf iso_root Blackrock.iso Blackrock.hdd
	$(MAKE) -C kernel clean

.PHONY: distclean
distclean: clean
	rm -rf limine ovmf-x64
	$(MAKE) -C kernel distclean
